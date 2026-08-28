// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyBluetoothManager.h"
#include "ArriettySerialController.h"
#include "ArriettyRideLog.h"
#include "GameFramework/Pawn.h"
#include "ArriettyTypes.h"
#include "ArriettyPawn.generated.h"

class UCameraComponent;
class UArriettyAlertWidget;
class UArriettyInstrumentWidget;
class UMotionControllerComponent;
class USceneComponent;
class UWidgetComponent;
UCLASS()
class ARRIETTYRUNTIME_API AArriettyPawn : public APawn
{
    GENERATED_BODY()

public:
    AArriettyPawn();
    virtual ~AArriettyPawn() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void ToggleVrSession();
    void RecenterHmdToBike();
    void QuitApplication();
    void StartRide();
    void StopRide(const TCHAR* LogEvent = TEXT("STOP"));
    void ToggleFlight();
    void ToggleInstrumentPanel();
    void SelectControlPreset(int32 PresetIndex);
    void StepControlPreset(int32 Step);
    void NavigateForward();
    void NavigateBackward();
    void TurnLeft();
    void TurnRight();

    bool IsVrSessionActive() const { return bVrSessionActive; }
    bool IsInstrumentVisible() const { return bInstrumentVisible; }
    bool IsRideActive() const;
    bool IsHmdAvailable() const;
    FString GetVrStatusText() const;
    FVector2D GetHmdForward() const;
    FVector2D GetBikeWorldForward() const;
    const FArriettyRideSnapshot& GetSnapshot() const { return Snapshot; }
    const FString& GetInstrumentAnchorStatus() const { return InstrumentAnchorStatus; }
    FString GetRideLogPath() const;

    double GetMoveStepMeters() const { return MoveStepMeters; }
    double GetTurnStepDegrees() const { return TurnStepDegrees; }
    double GetLapLengthMeters() const { return LapLengthMeters; }
    double GetPanelForwardOffsetMeters() const { return PanelForwardOffsetMeters; }
    double GetPanelSideOffsetMeters() const { return PanelSideOffsetMeters; }
    double GetPanelHeightOffsetMeters() const { return PanelHeightOffsetMeters; }
    double GetPanelScale() const { return PanelScale; }
    void SetMoveStepMeters(double Value);
    void SetTurnStepDegrees(double Value);
    void SetLapLengthMeters(double Value);
    void SetPanelForwardOffsetMeters(double Value);
    void SetPanelSideOffsetMeters(double Value);
    void SetPanelHeightOffsetMeters(double Value);
    void SetPanelScale(double Value);

private:
    bool TryStartVrSession();
    bool TryAlignHmdToBike();
    void RetryHmdAlignment();
    bool TryGetHmdTrackingForward(FVector2D& OutForward, double& OutYawDegrees) const;
    void ActivateVrSession();
    void InitializeVrAtStartup();
    void PumpBluetoothEvents();
    void PumpControllerEvents();
    void HandleControllerSample(const FArriettyControllerSample& Sample);
    void SetBrakeButtonHeld(bool bHeld);
    void RecoverTwoMeters();
    void ResetRecoveryTrail();
    void RecordRecoveryPose(double AdvanceMeters);
    void HandleCscSample(double ReceivedAtSeconds, const FArriettyCscSample& Sample);
    UMotionControllerComponent* ResolveSteeringController();
    void UpdateSteering();
    void MaybeBeginRiding();
    void AdvanceRide(float DeltaSeconds);
    void AdvanceHumanPoweredFlight(float DeltaSeconds, double NowSeconds);
    void ResetHumanPoweredFlight(double InitialAirspeedKmh = 0.0);
    void SyncHumanPoweredFlightSnapshot();
    void UpdateWorldTransform(bool bRequireRideSurface);
    bool ResolveRideSurfaceHeight(const FVector2D& PositionMeters, double& OutHeightMeters) const;
    void RefreshRideSurfaceMode();
    void MoveManual(double Direction);
    void SelectPreset1Input();
    void SelectPreset2Input();
    void SelectPreset3Input();
    void SelectPreset4Input();
    void StepPresetUpInput();
    void StepPresetDownInput();
    void CalibrateEyeHeight();
    void ResetInstrumentAnchor();
    void UpdateInstrumentAnchor();
    void UpdateInstrumentWidget();
    void ShowVrAlert(const FString& Message, double DurationSeconds = 3.0);
    void UpdateVrAlert();
    void PlayStartSound();
    void RecordTelemetry(const TCHAR* Event = TEXT("SAMPLE"));
    bool IsWheelStopped(double NowSeconds) const;
    bool IsLowSpeedCoastStopped() const;
    FVector ArriettyToWorld(const FVector2D& PositionMeters, double HeightMeters) const;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> VrOrigin;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UMotionControllerComponent> RightController;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UMotionControllerComponent> LeftController;

    UPROPERTY(Transient)
    TObjectPtr<UMotionControllerComponent> ActiveSteeringController;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UWidgetComponent> InstrumentComponent;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UWidgetComponent> AlertComponent;

    UPROPERTY(Transient)
    TObjectPtr<UArriettyInstrumentWidget> InstrumentWidget;

    UPROPERTY(Transient)
    TObjectPtr<UArriettyAlertWidget> AlertWidget;

    TUniquePtr<FArriettyBluetoothManager> Bluetooth;
    TUniquePtr<FArriettySerialController> SerialController;
    TUniquePtr<FArriettyRideLog> RideLog;
    FArriettyRideSnapshot Snapshot;
    FArriettyFlightState FlightState;

    FVector2D StartPositionMeters = FVector2D::ZeroVector;
    double StartHeadingDegrees = 0.0;
    double MoveStepMeters = Arrietty::DefaultMoveStepMeters;
    double TurnStepDegrees = Arrietty::DefaultTurnStepDegrees;
    double LapLengthMeters = 2605.0;
    double PanelForwardOffsetMeters = 0.0;
    double PanelSideOffsetMeters = 0.0;
    double PanelHeightOffsetMeters = 0.10;
    double PanelScale = 1.0;

    bool bVrSessionActive = false;
    bool bControllerInputInitialized = false;
    uint8 PreviousControllerButtonMask = 0;
    struct FRecoveryPose
    {
        FVector2D PositionMeters = FVector2D::ZeroVector;
        double HeadingDegrees = 0.0;
        double GroundHeightMeters = 0.0;
        double PathDistanceMeters = 0.0;
    };
    TArray<FRecoveryPose> RecoveryTrail;
    double RecoveryPathDistanceMeters = 0.0;
    double LastRecordedRecoveryDistanceMeters = 0.0;
    int32 VrStartupAttempts = 0;
    FTimerHandle VrStartupRetryTimer;
    int32 HmdAlignmentAttempts = 0;
    FTimerHandle HmdAlignmentRetryTimer;
    bool bHmdAligned = false;
    bool bInstrumentVisible = false;
    bool bWorldUsesRideSurfaces = false;
    bool bTrainerSignalReceived = false;
    bool bSteeringCalibrated = false;
    double SteeringCalibrationReadyAtSeconds = 0.0;
    bool bControllerTrackingLossAlerted = false;
    FQuat SteeringBaseline = FQuat::Identity;
    double FilteredSteeringDegrees = 0.0;
    FVector InstrumentAnchorLocalCentimeters = FVector(105.0, 0.0, 100.0);
    FString InstrumentAnchorStatus = TEXT("HIDDEN - Instrument panel is hidden");
    double AlertVisibleUntilSeconds = 0.0;

    double FtmsSpeedKmh = 0.0;
    double LastFtmsSampleSeconds = 0.0;
    double LastHeartRateSampleSeconds = 0.0;
    bool bWheelSignalReceived = false;
    TOptional<uint32> WheelRevolutions;
    TOptional<uint16> WheelEventTimeTicks;
    double LastWheelMotionSeconds = 0.0;
    double WheelPeriodSeconds = 0.0;
    double GroundHeightMeters = 0.0;
    double FpsAccumulatorSeconds = 0.0;
    int32 FpsAccumulatorFrames = 0;
};
