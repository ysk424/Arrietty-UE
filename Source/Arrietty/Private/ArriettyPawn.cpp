// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ArriettyPawn.h"

#include "ArriettyBluetoothManager.h"
#include "ArriettyInstrumentWidget.h"
#include "ArriettyRideLog.h"
#include "ArriettyTrainerProtocol.h"
#include "Async/Async.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "MotionControllerComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "IHeadMountedDisplay.h"
#include "StereoRendering.h"
#include "IXRTrackingSystem.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogArriettyRide, Log, All);

AArriettyPawn::AArriettyPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    VrOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
    VrOrigin->SetupAttachment(SceneRoot);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("HMD Camera"));
    Camera->SetupAttachment(VrOrigin);
    Camera->bLockToHmd = true;

    RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Right Controller"));
    RightController->SetupAttachment(VrOrigin);
    RightController->SetTrackingMotionSource(FName(TEXT("RightGrip")));
    RightController->PlayerIndex = 0;

    InstrumentComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("VR Instrument Panel"));
    InstrumentComponent->SetupAttachment(SceneRoot);
    InstrumentComponent->SetWidgetSpace(EWidgetSpace::World);
    InstrumentComponent->SetDrawSize(FVector2D(800.0, 360.0));
    InstrumentComponent->SetPivot(FVector2D(0.5, 0.5));
    InstrumentComponent->SetTwoSided(true);
    InstrumentComponent->SetBlendMode(EWidgetBlendMode::Transparent);
    InstrumentComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InstrumentComponent->SetVisibility(false);

    Snapshot.PositionMeters = StartPositionMeters;
    Snapshot.HeadingDegrees = StartHeadingDegrees;
    Snapshot.SelectedPreset = 5;
}

AArriettyPawn::~AArriettyPawn() = default;

void AArriettyPawn::BeginPlay()
{
    Super::BeginPlay();
    Bluetooth = MakeUnique<FArriettyBluetoothManager>();
    RideLog = MakeUnique<FArriettyRideLog>();
    InstrumentComponent->SetWidgetClass(UArriettyInstrumentWidget::StaticClass());
    InstrumentComponent->InitWidget();
    RefreshRideSurfaceMode();
    UpdateWorldTransform(false);
    InstrumentComponent->SetVisibility(false);

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        PlayerController->bShowMouseCursor = true;
        PlayerController->SetInputMode(FInputModeGameAndUI());
    }
    FTimerHandle SurfaceRefreshTimer;
    GetWorldTimerManager().SetTimer(SurfaceRefreshTimer, [this]
    {
        RefreshRideSurfaceMode();
        UpdateWorldTransform(false);
    }, 0.1f, false);

    // Start In VR asks OpenXR to create the session before gameplay.  Some
    // runtimes need a few frames after map load, so verify and retry briefly.
    GetWorldTimerManager().SetTimer(
        VrStartupRetryTimer,
        this,
        &AArriettyPawn::InitializeVrAtStartup,
        0.5f,
        true,
        0.1f);
}

void AArriettyPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopRide(TEXT("STOP"));
    if (Bluetooth)
    {
        Bluetooth->StopAndWait();
    }
    Super::EndPlay(EndPlayReason);
}

void AArriettyPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    PumpBluetoothEvents();
    UpdateSteering();
    MaybeBeginRiding();
    AdvanceRide(FMath::Min(DeltaSeconds, 0.25f));
    UpdateInstrumentAnchor();
    UpdateInstrumentWidget();

    FpsAccumulatorSeconds += DeltaSeconds;
    ++FpsAccumulatorFrames;
    if (FpsAccumulatorSeconds >= 0.5)
    {
        const double CurrentFps = FpsAccumulatorFrames / FpsAccumulatorSeconds;
        Snapshot.AverageFps = FMath::Lerp(Snapshot.AverageFps, CurrentFps, 0.35);
        FpsAccumulatorSeconds = 0.0;
        FpsAccumulatorFrames = 0;
    }
}

void AArriettyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &AArriettyPawn::StartRide);
    PlayerInputComponent->BindKey(EKeys::NumPadSeven, IE_Pressed, this, &AArriettyPawn::ToggleFlight);
    PlayerInputComponent->BindKey(EKeys::NumPadEight, IE_Pressed, this, &AArriettyPawn::NavigateForward);
    PlayerInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &AArriettyPawn::NavigateBackward);
    PlayerInputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &AArriettyPawn::TurnLeft);
    PlayerInputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &AArriettyPawn::TurnRight);
    PlayerInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &AArriettyPawn::SelectPreset1Input);
    PlayerInputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &AArriettyPawn::SelectPreset2Input);
    PlayerInputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &AArriettyPawn::SelectPreset3Input);
    PlayerInputComponent->BindKey(EKeys::NumPadNine, IE_Pressed, this, &AArriettyPawn::SelectPreset4Input);
    PlayerInputComponent->BindKey(EKeys::Add, IE_Pressed, this, &AArriettyPawn::StepPresetUpInput);
    PlayerInputComponent->BindKey(EKeys::Subtract, IE_Pressed, this, &AArriettyPawn::StepPresetDownInput);
    PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AArriettyPawn::QuitApplication);
}

bool AArriettyPawn::IsRideActive() const
{
    return Snapshot.Status == EArriettyRideStatus::Searching ||
        Snapshot.Status == EArriettyRideStatus::Connecting ||
        Snapshot.Status == EArriettyRideStatus::WaitingSteering ||
        Snapshot.Status == EArriettyRideStatus::Riding ||
        Snapshot.Status == EArriettyRideStatus::Stopping;
}

bool AArriettyPawn::IsHmdAvailable() const
{
    return GEngine != nullptr && GEngine->XRSystem.IsValid() &&
        GEngine->XRSystem->GetHMDDevice() != nullptr &&
        GEngine->XRSystem->GetHMDDevice()->IsHMDConnected();
}

FString AArriettyPawn::GetVrStatusText() const
{
    const bool bXrLoaded = GEngine != nullptr && GEngine->XRSystem.IsValid();
    const bool bStereoEnabled = GEngine != nullptr && GEngine->StereoRenderingDevice.IsValid() &&
        GEngine->StereoRenderingDevice->IsStereoEnabled();
    const FString RuntimeName = bXrLoaded
        ? GEngine->XRSystem->GetSystemName().ToString()
        : TEXT("not loaded");
    return FString::Printf(
        TEXT("VR: %s | HMD %s | stereo %s"),
        *RuntimeName,
        IsHmdAvailable() ? TEXT("connected") : TEXT("not detected"),
        bStereoEnabled ? TEXT("ON") : TEXT("OFF"));
}

FVector2D AArriettyPawn::GetHmdForward() const
{
    if (!Camera)
    {
        return FVector2D::ZeroVector;
    }
    const FVector Forward = Camera->GetForwardVector().GetSafeNormal2D();
    return FVector2D(Forward.X, -Forward.Y);
}

void AArriettyPawn::ToggleVrSession()
{
    if (bVrSessionActive)
    {
        StopRide(TEXT("BACK_TO_REAL_WORLD"));
        bInstrumentVisible = false;
        InstrumentComponent->SetVisibility(false);
        InstrumentAnchorStatus = TEXT("HIDDEN - Instrument panel is hidden");
        bVrSessionActive = false;
        if (GEngine && GEngine->StereoRenderingDevice.IsValid())
        {
            GEngine->StereoRenderingDevice->EnableStereo(false);
        }
        if (GEngine && GEngine->XRSystem.IsValid() && GEngine->XRSystem->GetHMDDevice())
        {
            GEngine->XRSystem->GetHMDDevice()->EnableHMD(false);
        }
        Snapshot.Message = TEXT("VR stopped. Press Dive into Secret World to start it again");
        return;
    }

    TryStartVrSession();
}

bool AArriettyPawn::TryStartVrSession()
{
    if (!IsHmdAvailable())
    {
        Snapshot.Message = TEXT("VR start failed: start SteamVR, make it the active OpenXR runtime, and connect the HMD");
        return false;
    }

    IHeadMountedDisplay* Hmd = GEngine->XRSystem->GetHMDDevice();
    if (!Hmd)
    {
        Snapshot.Message = TEXT("VR start failed: OpenXR did not provide an HMD device");
        return false;
    }
    Hmd->EnableHMD(true);
    if (!Hmd->IsHMDEnabled())
    {
        Snapshot.Message = TEXT("VR start failed: OpenXR could not enable the HMD");
        return false;
    }
    if (!GEngine->StereoRenderingDevice.IsValid())
    {
        Snapshot.Message = TEXT("VR start failed: the OpenXR stereo renderer is unavailable");
        return false;
    }

    const bool bStereoEnabled = GEngine->StereoRenderingDevice->IsStereoEnabled() ||
        GEngine->StereoRenderingDevice->EnableStereo(true);
    if (!bStereoEnabled)
    {
        Snapshot.Message = TEXT("VR start failed: OpenXR did not start stereo presentation");
        return false;
    }

    ActivateVrSession();
    return true;
}

void AArriettyPawn::ActivateVrSession()
{
    if (!GEngine || !GEngine->XRSystem.IsValid())
    {
        return;
    }
    GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
    GEngine->XRSystem->ResetOrientationAndPosition(0.0f);
    bVrSessionActive = true;
    bInstrumentVisible = true;
    ResetInstrumentAnchor();
    InstrumentComponent->SetVisibility(true);
    UpdateWorldTransform(false);
    CalibrateEyeHeight();
    GetWorldTimerManager().SetTimerForNextTick(this, &AArriettyPawn::CalibrateEyeHeight);
    FTimerHandle CalibrationTimer;
    GetWorldTimerManager().SetTimer(CalibrationTimer, this, &AArriettyPawn::CalibrateEyeHeight, 0.5f, false);
    Snapshot.Message = TEXT("VR active. Press Esc or Exit Arrietty to close the application");
}

void AArriettyPawn::InitializeVrAtStartup()
{
    ++VrStartupAttempts;
    if (TryStartVrSession() || VrStartupAttempts >= 10)
    {
        GetWorldTimerManager().ClearTimer(VrStartupRetryTimer);
    }
}

void AArriettyPawn::QuitApplication()
{
    StopRide(TEXT("EXIT_APPLICATION"));
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
    }
}

void AArriettyPawn::StartRide()
{
    if (IsRideActive())
    {
        Snapshot.Message = TEXT("Ride continues until Back to Real World");
        return;
    }
    if (!bVrSessionActive)
    {
        Snapshot.Status = EArriettyRideStatus::Error;
        Snapshot.Message = TEXT("Start the VR session before pressing Numpad 0");
        return;
    }
    if (!RideLog || !RideLog->Start())
    {
        Snapshot.Status = EArriettyRideStatus::Error;
        Snapshot.Message = TEXT("Could not create Saved/arrietty_ride.csv");
        return;
    }

    Snapshot.Status = EArriettyRideStatus::Searching;
    Snapshot.Message = TEXT("Searching for CYCPLUS T2");
    Snapshot.ControlStatus = FString::Printf(TEXT("REQUESTING P%d"), Snapshot.SelectedPreset);
    Snapshot.ControlMessage = TEXT("Requesting T2 control and flat-road preset");
    Snapshot.SpeedKmh = 0.0;
    Snapshot.FtmsSpeedKmh = 0.0;
    Snapshot.CadenceRpm = 0.0;
    Snapshot.PowerWatts = 0;
    Snapshot.DistanceMeters = 0.0;
    Snapshot.LapsCompleted = 0;
    Snapshot.AppliedPreset.Reset();
    Snapshot.bFlightEnabled = false;
    Snapshot.AltitudeMeters = 0.0;
    FtmsSpeedKmh = 0.0;
    LastFtmsSampleSeconds = 0.0;
    bTrainerSignalReceived = false;
    bSteeringCalibrated = false;
    FilteredSteeringDegrees = 0.0;
    bWheelSignalReceived = false;
    WheelRevolutions.Reset();
    WheelEventTimeTicks.Reset();
    LastWheelMotionSeconds = 0.0;
    WheelPeriodSeconds = 0.0;
    ResetInstrumentAnchor();
    Bluetooth->Start(Snapshot.SelectedPreset);
}

void AArriettyPawn::StopRide(const TCHAR* LogEvent)
{
    if (Bluetooth)
    {
        Bluetooth->RequestStop();
    }
    if (IsRideActive())
    {
        Snapshot.Status = EArriettyRideStatus::Stopping;
        Snapshot.Message = TEXT("Stopping trainer");
    }
    Snapshot.SpeedKmh = 0.0;
    Snapshot.FtmsSpeedKmh = 0.0;
    Snapshot.bFlightEnabled = false;
    Snapshot.AltitudeMeters = 0.0;
    Snapshot.bSteeringTracking = false;
    UpdateWorldTransform(false);
    if (RideLog && RideLog->IsActive())
    {
        RideLog->Stop(LogEvent, &Snapshot);
    }
}

void AArriettyPawn::ToggleFlight()
{
    if (!IsRideActive())
    {
        Snapshot.Message = TEXT("Start the ride before enabling flight");
        return;
    }
    Snapshot.bFlightEnabled = !Snapshot.bFlightEnabled;
    if (!Snapshot.bFlightEnabled)
    {
        Snapshot.AltitudeMeters = 0.0;
    }
    UpdateWorldTransform(false);
}

void AArriettyPawn::ToggleInstrumentPanel()
{
    bInstrumentVisible = !bInstrumentVisible;
    InstrumentComponent->SetVisibility(bInstrumentVisible);
    if (bInstrumentVisible)
    {
        ResetInstrumentAnchor();
    }
    else
    {
        InstrumentAnchorStatus = TEXT("HIDDEN - Instrument panel is hidden");
    }
}

void AArriettyPawn::SelectControlPreset(int32 PresetIndex)
{
    if (ArriettyTrainerProtocol::FindPreset(PresetIndex) == nullptr)
    {
        return;
    }
    Snapshot.SelectedPreset = PresetIndex;
    if (IsRideActive() && Snapshot.Status != EArriettyRideStatus::Stopping)
    {
        Bluetooth->RequestPreset(PresetIndex);
        Snapshot.ControlStatus = FString::Printf(TEXT("SETTING P%d"), PresetIndex);
        Snapshot.ControlMessage = TEXT("Applying flat-road rolling resistance");
    }
    else
    {
        Snapshot.ControlStatus = FString::Printf(TEXT("SELECTED P%d"), PresetIndex);
        Snapshot.ControlMessage = TEXT("The preset applies on the next ride");
    }
}

void AArriettyPawn::StepControlPreset(int32 Step)
{
    SelectControlPreset(FMath::Clamp(Snapshot.SelectedPreset + Step, 1, 7));
}

void AArriettyPawn::SelectPreset1Input() { SelectControlPreset(1); }
void AArriettyPawn::SelectPreset2Input() { SelectControlPreset(2); }
void AArriettyPawn::SelectPreset3Input() { SelectControlPreset(3); }
void AArriettyPawn::SelectPreset4Input() { SelectControlPreset(4); }
void AArriettyPawn::StepPresetUpInput() { StepControlPreset(1); }
void AArriettyPawn::StepPresetDownInput() { StepControlPreset(-1); }

void AArriettyPawn::NavigateForward()
{
    MoveManual(1.0);
}

void AArriettyPawn::NavigateBackward()
{
    MoveManual(-1.0);
}

void AArriettyPawn::TurnLeft()
{
    Snapshot.HeadingDegrees = FMath::UnwindDegrees(Snapshot.HeadingDegrees + TurnStepDegrees);
    StartHeadingDegrees = Snapshot.HeadingDegrees;
    UpdateWorldTransform(false);
}

void AArriettyPawn::TurnRight()
{
    Snapshot.HeadingDegrees = FMath::UnwindDegrees(Snapshot.HeadingDegrees - TurnStepDegrees);
    StartHeadingDegrees = Snapshot.HeadingDegrees;
    UpdateWorldTransform(false);
}

void AArriettyPawn::MoveManual(double Direction)
{
    FVector2D Forward;
    if (bVrSessionActive)
    {
        const FVector WorldForward = Camera->GetForwardVector().GetSafeNormal2D();
        Forward = FVector2D(WorldForward.X, -WorldForward.Y).GetSafeNormal();
    }
    else
    {
        const double HeadingRadians = FMath::DegreesToRadians(Snapshot.HeadingDegrees);
        Forward = FVector2D(FMath::Cos(HeadingRadians), FMath::Sin(HeadingRadians));
    }
    Snapshot.PositionMeters += Forward * (Direction * MoveStepMeters);
    StartPositionMeters = Snapshot.PositionMeters;
    UpdateWorldTransform(false);
}

void AArriettyPawn::PumpBluetoothEvents()
{
    if (!Bluetooth)
    {
        return;
    }
    FArriettyBluetoothEvent Event;
    while (Bluetooth->DequeueEvent(Event))
    {
        if (Event.Generation != Bluetooth->GetGeneration())
        {
            continue;
        }
        switch (Event.Type)
        {
        case EArriettyBluetoothEventType::Status:
            Snapshot.Status = Event.Status;
            Snapshot.Message = Event.Message;
            break;
        case EArriettyBluetoothEventType::Connected:
            Snapshot.Message = TEXT("T2 flat-road control active; waiting for FTMS");
            break;
        case EArriettyBluetoothEventType::ControlReady:
            Snapshot.AppliedPreset = Event.PresetIndex;
            Snapshot.ControlStatus = FString::Printf(TEXT("FLAT P%d"), Event.PresetIndex);
            if (const FArriettyControlPreset* Preset = ArriettyTrainerProtocol::FindPreset(Event.PresetIndex))
            {
                Snapshot.ControlMessage = FString::Printf(
                    TEXT("P%d %s: grade 0%%; Crr %.4f; Cw 0.51 kg/m"),
                    Preset->Index, Preset->Label, Preset->RollingResistance);
            }
            break;
        case EArriettyBluetoothEventType::TrainerSample:
            LastFtmsSampleSeconds = Event.ReceivedAtSeconds;
            if (Event.TrainerSample.SpeedKmh.IsSet()) FtmsSpeedKmh = FMath::Max(0.0, Event.TrainerSample.SpeedKmh.GetValue());
            if (Event.TrainerSample.CadenceRpm.IsSet()) Snapshot.CadenceRpm = FMath::Max(0.0, Event.TrainerSample.CadenceRpm.GetValue());
            if (Event.TrainerSample.PowerWatts.IsSet()) Snapshot.PowerWatts = FMath::Max(0, Event.TrainerSample.PowerWatts.GetValue());
            Snapshot.FtmsSpeedKmh = FtmsSpeedKmh;
            bTrainerSignalReceived = true;
            Snapshot.SpeedKmh = ArriettyTrainerProtocol::EffectiveSpeedKmh(
                FPlatformTime::Seconds(), LastFtmsSampleSeconds, FtmsSpeedKmh,
                Snapshot.CadenceRpm, bWheelSignalReceived, LastWheelMotionSeconds, WheelPeriodSeconds);
            MaybeBeginRiding();
            RecordTelemetry();
            break;
        case EArriettyBluetoothEventType::CscSample:
            HandleCscSample(Event.ReceivedAtSeconds, Event.CscSample);
            break;
        case EArriettyBluetoothEventType::CscUnavailable:
            Snapshot.Message = TEXT("CSC wheel rotation unavailable; using FTMS speed only");
            break;
        case EArriettyBluetoothEventType::Error:
            Snapshot.Status = EArriettyRideStatus::Error;
            Snapshot.Message = Event.Message;
            Snapshot.ControlStatus = TEXT("ERROR");
            Snapshot.ControlMessage = Event.Message;
            Snapshot.SpeedKmh = 0.0;
            Snapshot.FtmsSpeedKmh = 0.0;
            Snapshot.AppliedPreset.Reset();
            Snapshot.bFlightEnabled = false;
            Snapshot.AltitudeMeters = 0.0;
            if (RideLog && RideLog->IsActive()) RideLog->Stop(TEXT("ERROR"), &Snapshot);
            break;
        case EArriettyBluetoothEventType::WorkerStopped:
            if (Snapshot.Status == EArriettyRideStatus::Stopping)
            {
                Snapshot.Status = EArriettyRideStatus::Idle;
                Snapshot.Message = TEXT("Trainer stopped");
                Snapshot.ControlStatus = TEXT("IDLE");
                Snapshot.ControlMessage = FString::Printf(
                    TEXT("P%d selected for the next ride"), Snapshot.SelectedPreset);
                Snapshot.AppliedPreset.Reset();
            }
            break;
        default:
            break;
        }
    }
}

void AArriettyPawn::HandleCscSample(double ReceivedAtSeconds, const FArriettyCscSample& Sample)
{
    if (!Sample.WheelRevolutions.IsSet() || !Sample.WheelEventTimeTicks.IsSet())
    {
        return;
    }
    const TOptional<uint32> PreviousRevolutions = WheelRevolutions;
    const TOptional<uint16> PreviousTicks = WheelEventTimeTicks;
    bWheelSignalReceived = true;
    WheelRevolutions = Sample.WheelRevolutions;
    WheelEventTimeTicks = Sample.WheelEventTimeTicks;
    if (!PreviousRevolutions.IsSet() || !PreviousTicks.IsSet())
    {
        LastWheelMotionSeconds = ReceivedAtSeconds;
        return;
    }
    const uint32 RevolutionDelta = Sample.WheelRevolutions.GetValue() - PreviousRevolutions.GetValue();
    if (RevolutionDelta == 0)
    {
        return;
    }
    const uint16 TickDelta = static_cast<uint16>(Sample.WheelEventTimeTicks.GetValue() - PreviousTicks.GetValue());
    if (TickDelta > 0 && RevolutionDelta < 1000)
    {
        const double Period = TickDelta / 1024.0 / RevolutionDelta;
        if (Period >= 0.01 && Period <= 30.0)
        {
            WheelPeriodSeconds = Period;
        }
    }
    LastWheelMotionSeconds = ReceivedAtSeconds;
}

void AArriettyPawn::UpdateSteering()
{
    if (!IsRideActive() || Snapshot.Status == EArriettyRideStatus::Stopping)
    {
        Snapshot.bSteeringTracking = false;
        Snapshot.RawSteeringDegrees = 0.0;
        Snapshot.EffectiveSteeringDegrees = 0.0;
        return;
    }
    const bool bTracked = RightController && RightController->IsTracked();
    Snapshot.bSteeringTracking = bTracked;
    if (!bTracked)
    {
        Snapshot.RawSteeringDegrees = 0.0;
        Snapshot.EffectiveSteeringDegrees = 0.0;
        if (Snapshot.Status == EArriettyRideStatus::Riding)
        {
            Snapshot.Message = TEXT("Ride paused; right controller tracking was lost");
        }
        return;
    }

    // Steering is measured in tracking-space. A world-space rotation would feed
    // the bicycle's own turn back into the controller angle on the next frame.
    const FQuat Current = RightController->GetRelativeRotation().Quaternion();
    if (!bSteeringCalibrated)
    {
        SteeringBaseline = Current;
        FilteredSteeringDegrees = 0.0;
        bSteeringCalibrated = true;
    }
    else
    {
        const FQuat Delta = Current * SteeringBaseline.Inverse();
        const double RawDegrees = -Delta.Rotator().Yaw;
        FilteredSteeringDegrees += 0.25 * (RawDegrees - FilteredSteeringDegrees);
    }
    Snapshot.RawSteeringDegrees = FilteredSteeringDegrees;
    Snapshot.EffectiveSteeringDegrees = ArriettyTrainerProtocol::EffectiveSteeringDegrees(FilteredSteeringDegrees);
    if (Snapshot.Status == EArriettyRideStatus::Riding && Snapshot.Message.StartsWith(TEXT("Ride paused; right controller")))
    {
        Snapshot.Message = TEXT("Right controller recovered; steering is active");
    }
}

void AArriettyPawn::MaybeBeginRiding()
{
    if (!bTrainerSignalReceived || !Snapshot.bSteeringTracking)
    {
        if (bTrainerSignalReceived && Snapshot.Status != EArriettyRideStatus::Riding)
        {
            Snapshot.Status = EArriettyRideStatus::WaitingSteering;
            Snapshot.Message = TEXT("T2 received; waiting for the right controller");
        }
        return;
    }
    if (Snapshot.Status != EArriettyRideStatus::Riding)
    {
        Snapshot.Status = EArriettyRideStatus::Riding;
        Snapshot.Message = TEXT("T2 and right controller received; steering is active");
        PlayStartSound();
    }
}

void AArriettyPawn::AdvanceRide(float DeltaSeconds)
{
    if (Snapshot.Status != EArriettyRideStatus::Riding)
    {
        return;
    }
    const double Now = FPlatformTime::Seconds();
    Snapshot.SpeedKmh = ArriettyTrainerProtocol::EffectiveSpeedKmh(
        Now, LastFtmsSampleSeconds, FtmsSpeedKmh, Snapshot.CadenceRpm,
        bWheelSignalReceived, LastWheelMotionSeconds, WheelPeriodSeconds);
    if (!Snapshot.bSteeringTracking)
    {
        return;
    }
    if (IsWheelStopped(Now) && FtmsSpeedKmh > 0.0)
    {
        Snapshot.Message = TEXT("Stopped; CSC wheel rotation is stationary");
    }
    else if (IsLowSpeedCoastStopped())
    {
        Snapshot.Message = TEXT("Stopped; coasting at or below 5.0 km/h");
    }
    else if (Snapshot.SpeedKmh > 0.0 && Snapshot.Message.StartsWith(TEXT("Stopped;")))
    {
        Snapshot.Message = TEXT("Trainer motion received; steering is active");
    }

    Snapshot.AltitudeMeters = Snapshot.bFlightEnabled
        ? ArriettyTrainerProtocol::AltitudeForSpeed(Snapshot.SpeedKmh)
        : 0.0;
    const double AdvanceMeters = Snapshot.SpeedKmh / 3.6 * DeltaSeconds;
    if (AdvanceMeters <= 0.0)
    {
        UpdateWorldTransform(false);
        return;
    }

    const double SteeringRadians = FMath::DegreesToRadians(Snapshot.EffectiveSteeringDegrees);
    const double TurnRadians = AdvanceMeters / Arrietty::WheelbaseMeters * FMath::Tan(SteeringRadians);
    const double CurrentHeadingRadians = FMath::DegreesToRadians(Snapshot.HeadingDegrees);
    const double MidpointHeading = CurrentHeadingRadians + TurnRadians * 0.5;
    const FVector2D NextPosition = Snapshot.PositionMeters + FVector2D(
        FMath::Cos(MidpointHeading), FMath::Sin(MidpointHeading)) * AdvanceMeters;

    double NextGroundHeight = GroundHeightMeters;
    if (bWorldUsesRideSurfaces && !ResolveRideSurfaceHeight(NextPosition, NextGroundHeight))
    {
        Snapshot.Message = TEXT("Ride paused; no ride surface under the bicycle");
        UpdateWorldTransform(false);
        return;
    }
    if (Snapshot.Message.StartsWith(TEXT("Ride paused; no ride surface")))
    {
        Snapshot.Message = TEXT("Ride surface recovered; steering is active");
    }
    GroundHeightMeters = NextGroundHeight;
    Snapshot.PositionMeters = NextPosition;
    Snapshot.DistanceMeters += AdvanceMeters;
    Snapshot.HeadingDegrees = FMath::UnwindDegrees(
        Snapshot.HeadingDegrees + FMath::RadiansToDegrees(TurnRadians));
    Snapshot.LapsCompleted = ArriettyTrainerProtocol::CompletedLaps(
        Snapshot.DistanceMeters, LapLengthMeters);
    UpdateWorldTransform(false);
}

void AArriettyPawn::RefreshRideSurfaceMode()
{
    bWorldUsesRideSurfaces = false;
    for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
    {
        if (It->GetWorld() == GetWorld() &&
            (It->ComponentHasTag(FName(Arrietty::RideSurfaceTag)) ||
             (It->GetOwner() && It->GetOwner()->ActorHasTag(FName(Arrietty::RideSurfaceTag)))))
        {
            bWorldUsesRideSurfaces = true;
            break;
        }
    }
}

bool AArriettyPawn::ResolveRideSurfaceHeight(
    const FVector2D& PositionMeters,
    double& OutHeightMeters) const
{
    if (!GetWorld())
    {
        return false;
    }
    const FVector Start = ArriettyToWorld(PositionMeters, 10000.0);
    const FVector End = ArriettyToWorld(PositionMeters, -10000.0);
    TArray<FHitResult> Hits;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ArriettyRideSurface), false, this);
    if (!GetWorld()->LineTraceMultiByChannel(Hits, Start, End, ECC_Visibility, QueryParams))
    {
        return false;
    }
    bool bFound = false;
    double HighestMeters = -DBL_MAX;
    for (const FHitResult& Hit : Hits)
    {
        const bool bTaggedComponent = Hit.Component.IsValid() &&
            Hit.Component->ComponentHasTag(FName(Arrietty::RideSurfaceTag));
        const bool bTaggedActor = Hit.GetActor() &&
            Hit.GetActor()->ActorHasTag(FName(Arrietty::RideSurfaceTag));
        if (bTaggedComponent || bTaggedActor)
        {
            HighestMeters = FMath::Max(HighestMeters, Hit.ImpactPoint.Z / 100.0);
            bFound = true;
        }
    }
    if (bFound)
    {
        OutHeightMeters = HighestMeters;
    }
    return bFound;
}

void AArriettyPawn::UpdateWorldTransform(bool bRequireRideSurface)
{
    double ResolvedGround = GroundHeightMeters;
    if (bWorldUsesRideSurfaces)
    {
        if (ResolveRideSurfaceHeight(Snapshot.PositionMeters, ResolvedGround))
        {
            GroundHeightMeters = ResolvedGround;
        }
        else if (bRequireRideSurface)
        {
            return;
        }
    }
    else
    {
        GroundHeightMeters = 0.0;
    }
    SetActorLocationAndRotation(
        ArriettyToWorld(Snapshot.PositionMeters, GroundHeightMeters + Snapshot.AltitudeMeters),
        FRotator(0.0, -Snapshot.HeadingDegrees, 0.0));
}

FVector AArriettyPawn::ArriettyToWorld(const FVector2D& PositionMeters, double HeightMeters) const
{
    return FVector(PositionMeters.X * 100.0, -PositionMeters.Y * 100.0, HeightMeters * 100.0);
}

void AArriettyPawn::CalibrateEyeHeight()
{
    if (!bVrSessionActive || !Camera || !VrOrigin)
    {
        return;
    }
    const double CurrentEyeHeight = Camera->GetComponentLocation().Z - GetActorLocation().Z;
    const double Correction = Arrietty::EyeHeightMeters * 100.0 - CurrentEyeHeight;
    FVector OriginLocation = VrOrigin->GetRelativeLocation();
    OriginLocation.Z += Correction;
    VrOrigin->SetRelativeLocation(OriginLocation);
}

void AArriettyPawn::ResetInstrumentAnchor()
{
    bInstrumentAnchorCalibrated = false;
    InstrumentAnchorLocalCentimeters = FVector(68.0, 0.0, 102.0);
    InstrumentAnchorStatus = TEXT("VIRTUAL STEM - Waiting to calibrate the right controller position");
}

void AArriettyPawn::UpdateInstrumentAnchor()
{
    if (!bInstrumentVisible || !InstrumentComponent)
    {
        return;
    }
    if (!bInstrumentAnchorCalibrated && RightController && RightController->IsTracked())
    {
        InstrumentAnchorLocalCentimeters = GetActorTransform().InverseTransformPosition(
            RightController->GetComponentLocation());
        bInstrumentAnchorCalibrated = true;
        InstrumentAnchorStatus = TEXT("CALIBRATED RIGHT OPENXR GRIP - Controller position captured once; tracking jitter is ignored");
    }
    FVector Location = InstrumentAnchorLocalCentimeters;
    Location.X += PanelForwardOffsetMeters * 100.0;
    Location.Y += PanelSideOffsetMeters * 100.0;
    Location.Z += PanelHeightOffsetMeters * 100.0;
    InstrumentComponent->SetRelativeLocation(Location);
    InstrumentComponent->SetRelativeRotation(FRotator(-24.0, 180.0, 0.0));
    const float PixelScale = static_cast<float>(0.04 * PanelScale);
    InstrumentComponent->SetRelativeScale3D(FVector(PixelScale));
}

void AArriettyPawn::UpdateInstrumentWidget()
{
    if (!bInstrumentVisible || !InstrumentComponent)
    {
        return;
    }
    if (UArriettyInstrumentWidget* Widget = Cast<UArriettyInstrumentWidget>(InstrumentComponent->GetWidget()))
    {
        Widget->SetRideSnapshot(Snapshot);
    }
}

void AArriettyPawn::PlayStartSound()
{
#if PLATFORM_WINDOWS
    Async(EAsyncExecution::ThreadPool, [] { ::Beep(1200, 700); });
#endif
}

void AArriettyPawn::RecordTelemetry(const TCHAR* Event)
{
    if (RideLog && RideLog->IsActive())
    {
        RideLog->Record(
            Event, Snapshot, FtmsSpeedKmh, WheelRevolutions, WheelEventTimeTicks,
            IsWheelStopped(FPlatformTime::Seconds()), IsLowSpeedCoastStopped());
    }
}

bool AArriettyPawn::IsWheelStopped(double NowSeconds) const
{
    return bWheelSignalReceived &&
        NowSeconds - LastWheelMotionSeconds > ArriettyTrainerProtocol::WheelStopTimeoutSeconds(WheelPeriodSeconds);
}

bool AArriettyPawn::IsLowSpeedCoastStopped() const
{
    return FtmsSpeedKmh > 0.0 && FtmsSpeedKmh <= Arrietty::CoastStopSpeedKmh && Snapshot.CadenceRpm <= 0.0;
}

FString AArriettyPawn::GetRideLogPath() const
{
    return RideLog ? RideLog->GetPath() : FString();
}

void AArriettyPawn::SetMoveStepMeters(double Value)
{
    MoveStepMeters = FMath::Clamp(Value, 0.01, 10.0);
}

void AArriettyPawn::SetTurnStepDegrees(double Value)
{
    TurnStepDegrees = FMath::Clamp(Value, 0.1, 90.0);
}

void AArriettyPawn::SetLapLengthMeters(double Value)
{
    LapLengthMeters = FMath::Clamp(Value, 1.0, 10000.0);
    Snapshot.LapsCompleted = ArriettyTrainerProtocol::CompletedLaps(Snapshot.DistanceMeters, LapLengthMeters);
}

void AArriettyPawn::SetPanelForwardOffsetMeters(double Value)
{
    PanelForwardOffsetMeters = FMath::Clamp(Value, -0.5, 0.5);
}

void AArriettyPawn::SetPanelSideOffsetMeters(double Value)
{
    PanelSideOffsetMeters = FMath::Clamp(Value, -0.5, 0.5);
}

void AArriettyPawn::SetPanelHeightOffsetMeters(double Value)
{
    PanelHeightOffsetMeters = FMath::Clamp(Value, -0.5, 0.5);
}

void AArriettyPawn::SetPanelScale(double Value)
{
    PanelScale = FMath::Clamp(Value, 0.5, 2.0);
}
