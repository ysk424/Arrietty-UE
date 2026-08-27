// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyControlWidget.h"

#include "ArriettyPawn.h"
#include "ArriettyTrainerProtocol.h"
#include "ArriettyTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
void ConfigureText(UTextBlock* Text, int32 Size, const FLinearColor& Color)
{
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = Size;
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(Color));
    Text->SetAutoWrapText(true);
}

void AddSectionTitle(UWidgetTree* Tree, UVerticalBox* Parent, const FString& Label)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Label));
    ConfigureText(Text, 17, FLinearColor(0.35f, 0.95f, 0.55f));
    if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Text))
    {
        Slot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 4.0f));
    }
}
}

void UArriettyControlWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = Canvas;
    UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(FLinearColor(0.008f, 0.012f, 0.018f, 0.93f));
    Background->SetPadding(FMargin(16.0f));
    UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Background);
    BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 1.0f));
    BackgroundSlot->SetOffsets(FMargin(12.0f, 12.0f, 500.0f, -24.0f));

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    Background->SetContent(Scroll);
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(Root);

    UTextBlock* Title = AddText(Root, TEXT("Arrietty"), 28, FLinearColor(0.45f, 1.0f, 0.60f));
    Title->SetJustification(ETextJustify::Center);
    UTextBlock* Version = AddText(Root, FString::Printf(TEXT("Version v%s / Unreal Engine 5.8.2"), Arrietty::Version), 13, FLinearColor(0.7f, 0.75f, 0.8f));
    Version->SetJustification(ETextJustify::Center);

    UButton* VrButton = AddButton(Root, TEXT("Dive into Secret World"), VrButtonLabel);
    VrButton->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnToggleVr);
    VrStatusText = AddText(Root, TEXT("VR: initializing"), 13, FLinearColor(0.55f, 0.75f, 0.9f));
    TObjectPtr<UTextBlock> ExitButtonLabel;
    UButton* ExitButton = AddButton(Root, TEXT("Exit Arrietty (Esc)"), ExitButtonLabel);
    ExitButton->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnExitApplication);

    AddSectionTitle(WidgetTree, Root, TEXT("Start Pose"));
    StartPoseText = AddText(Root, TEXT("X 0.00 m   Y -320.00 m   Z 1.50 m"));
    AddText(Root, TEXT("Numpad 8 / 2: Forward / Back"), 13, FLinearColor(0.75f, 0.8f, 0.85f));
    AddText(Root, TEXT("Numpad 4 / 6: Turn Left / Right"), 13, FLinearColor(0.75f, 0.8f, 0.85f));
    AddText(Root, TEXT("Numpad 8 / 2 follows the current HMD view"), 13, FLinearColor(0.75f, 0.8f, 0.85f));
    HmdForwardText = AddText(Root, TEXT("HMD Forward --"), 13, FLinearColor(0.55f, 0.75f, 0.9f));
    TObjectPtr<UTextBlock> RecenterButtonLabel;
    UButton* RecenterButton = AddButton(Root, TEXT("Align HMD to Bike (Numpad .)"), RecenterButtonLabel);
    RecenterButton->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnRecenterHmd);
    AArriettyPawn* InitialPawn = FindArriettyPawn();
    USpinBox* Move = AddSetting(Root, TEXT("Move (m)"), InitialPawn ? InitialPawn->GetMoveStepMeters() : 0.5, 0.01, 10.0);
    Move->OnValueChanged.AddDynamic(this, &UArriettyControlWidget::OnMoveChanged);
    USpinBox* Turn = AddSetting(Root, TEXT("Turn (degrees)"), InitialPawn ? InitialPawn->GetTurnStepDegrees() : 5.0, 0.1, 90.0);
    Turn->OnValueChanged.AddDynamic(this, &UArriettyControlWidget::OnTurnChanged);

    AddSectionTitle(WidgetTree, Root, TEXT("CYCPLUS T2"));
    AddText(Root, TEXT("Ride direction follows Start Direction, not HMD view"), 13, FLinearColor(0.75f, 0.8f, 0.85f));
    USpinBox* Lap = AddSetting(Root, TEXT("Lap (m)"), InitialPawn ? InitialPawn->GetLapLengthMeters() : 2605.0, 1.0, 10000.0);
    Lap->OnValueChanged.AddDynamic(this, &UArriettyControlWidget::OnLapChanged);
    UButton* RideButton = AddButton(Root, TEXT("Start Ride (Numpad 0)"), RideButtonLabel);
    RideButton->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnStartRide);
    RideStatusText = AddText(Root, TEXT("Status: IDLE"));
    RideMessageText = AddText(Root, TEXT("Press Numpad 0 when the T2 is awake"), 13, FLinearColor(0.9f, 0.9f, 0.65f));
    ControlStatusText = AddText(Root, TEXT("T2 Control: IDLE"), 13);

    UHorizontalBox* PresetRow1 = WidgetTree->ConstructWidget<UHorizontalBox>();
    UHorizontalBox* PresetRow2 = WidgetTree->ConstructWidget<UHorizontalBox>();
    Root->AddChildToVerticalBox(PresetRow1);
    Root->AddChildToVerticalBox(PresetRow2);
    const TArray<FArriettyControlPreset>& Presets = ArriettyTrainerProtocol::Presets();
    for (int32 Index = 0; Index < Presets.Num(); ++Index)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(FString::Printf(TEXT("P%d %s"), Presets[Index].Index, Presets[Index].Label)));
        ConfigureText(Label, 11, FLinearColor::White);
        Button->SetContent(Label);
        UHorizontalBox* Row = Index < 4 ? PresetRow1 : PresetRow2;
        UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button);
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ButtonSlot->SetPadding(FMargin(2.0f));
        PresetButtons.Add(Button);
    }
    PresetButtons[0]->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnPreset1);
    PresetButtons[1]->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnPreset2);
    PresetButtons[2]->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnPreset3);
    PresetButtons[3]->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnPreset4);
    PresetButtons[4]->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnPreset5);
    PresetButtons[5]->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnPreset6);
    PresetButtons[6]->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnPreset7);
    AddText(Root, TEXT("Resistance: 1 / 3 / 5 / 9, then Numpad +/-"), 12, FLinearColor(0.75f, 0.8f, 0.85f));
    TelemetryText = AddText(Root, TEXT("0.00 km/h   0 rpm   0 W"), 16, FLinearColor(0.25f, 1.0f, 0.40f));
    DistanceText = AddText(Root, TEXT("Distance 0.0 m   Laps completed 0"));
    LogText = AddText(Root, TEXT("Log: Saved/arrietty_ride.csv"), 12, FLinearColor(0.7f, 0.75f, 0.8f));

    UButton* FlightButton = AddButton(Root, TEXT("Enable Flight (Numpad 7)"), FlightButtonLabel);
    FlightButton->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnToggleFlight);
    FlightStatusText = AddText(Root, TEXT("Mode: GROUND   Altitude 0.0 m"));

    AddSectionTitle(WidgetTree, Root, TEXT("VR Instrument Panel"));
    UButton* InstrumentButton = AddButton(Root, TEXT("Show Panel Preview"), InstrumentButtonLabel);
    InstrumentButton->OnClicked.AddDynamic(this, &UArriettyControlWidget::OnToggleInstrument);
    InstrumentStatusText = AddText(Root, TEXT("Anchor: HIDDEN"), 12, FLinearColor(0.75f, 0.8f, 0.85f));
    USpinBox* Forward = AddSetting(Root, TEXT("Forward (m)"), 0.0, -0.5, 0.5);
    USpinBox* Side = AddSetting(Root, TEXT("Side (m)"), 0.0, -0.5, 0.5);
    USpinBox* Height = AddSetting(Root, TEXT("Height (m)"), 0.10, -0.5, 0.5);
    USpinBox* Scale = AddSetting(Root, TEXT("Scale"), 1.0, 0.5, 2.0);
    Forward->OnValueChanged.AddDynamic(this, &UArriettyControlWidget::OnPanelForwardChanged);
    Side->OnValueChanged.AddDynamic(this, &UArriettyControlWidget::OnPanelSideChanged);
    Height->OnValueChanged.AddDynamic(this, &UArriettyControlWidget::OnPanelHeightChanged);
    Scale->OnValueChanged.AddDynamic(this, &UArriettyControlWidget::OnPanelScaleChanged);

    AddSectionTitle(WidgetTree, Root, TEXT("Right Controller Steering"));
    SteeringStatusText = AddText(Root, TEXT("Status: IDLE"), 13);
    AddText(Root, FString::Printf(TEXT("ID: %s"), Arrietty::RightControllerSerial), 12, FLinearColor(0.7f, 0.75f, 0.8f));
    PerformanceText = AddText(Root, TEXT("Performance: 60.0 FPS (target 60)"), 13, FLinearColor(0.25f, 1.0f, 0.40f));

    Refresh();
}

void UArriettyControlWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    TimeSinceRefresh += InDeltaTime;
    if (TimeSinceRefresh >= 0.10)
    {
        TimeSinceRefresh = 0.0;
        Refresh();
    }
}

AArriettyPawn* UArriettyControlWidget::FindArriettyPawn() const
{
    return GetOwningPlayer() ? Cast<AArriettyPawn>(GetOwningPlayer()->GetPawn()) : nullptr;
}

UTextBlock* UArriettyControlWidget::AddText(
    UVerticalBox* Parent,
    const FString& Text,
    int32 Size,
    const FLinearColor& Color)
{
    UTextBlock* Widget = WidgetTree->ConstructWidget<UTextBlock>();
    Widget->SetText(FText::FromString(Text));
    ConfigureText(Widget, Size, Color);
    if (UVerticalBoxSlot* TextSlot = Parent->AddChildToVerticalBox(Widget))
    {
        TextSlot->SetPadding(FMargin(2.0f));
    }
    return Widget;
}

UButton* UArriettyControlWidget::AddButton(
    UVerticalBox* Parent,
    const FString& Label,
    TObjectPtr<UTextBlock>& OutLabel)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    OutLabel = WidgetTree->ConstructWidget<UTextBlock>();
    OutLabel->SetText(FText::FromString(Label));
    OutLabel->SetJustification(ETextJustify::Center);
    ConfigureText(OutLabel, 15, FLinearColor::White);
    Button->SetContent(OutLabel);
    if (UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button))
    {
        ButtonSlot->SetPadding(FMargin(2.0f, 5.0f));
    }
    return Button;
}

USpinBox* UArriettyControlWidget::AddSetting(
    UVerticalBox* Parent,
    const FString& Label,
    double Value,
    double Min,
    double Max)
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    Parent->AddChildToVerticalBox(Row);
    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>();
    LabelText->SetText(FText::FromString(Label));
    ConfigureText(LabelText, 13, FLinearColor(0.85f, 0.88f, 0.92f));
    UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    USpinBox* Spin = WidgetTree->ConstructWidget<USpinBox>();
    Spin->SetMinValue(Min);
    Spin->SetMaxValue(Max);
    Spin->SetMinSliderValue(Min);
    Spin->SetMaxSliderValue(Max);
    Spin->SetDelta((Max - Min) / 100.0);
    Spin->SetValue(Value);
    UHorizontalBoxSlot* SpinSlot = Row->AddChildToHorizontalBox(Spin);
    SpinSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    return Spin;
}

void UArriettyControlWidget::Refresh()
{
    AArriettyPawn* Pawn = FindArriettyPawn();
    if (!Pawn || !StartPoseText)
    {
        return;
    }
    const FArriettyRideSnapshot& State = Pawn->GetSnapshot();
    VrButtonLabel->SetText(FText::FromString(
        Pawn->IsVrSessionActive() ? TEXT("Back to Real World") : TEXT("Dive into Secret World")));
    VrStatusText->SetText(FText::FromString(Pawn->GetVrStatusText()));
    StartPoseText->SetText(FText::FromString(FString::Printf(
        TEXT("X %.2f m   Y %.2f m   Z %.2f m\nDirection %.1f degrees"),
        State.PositionMeters.X,
        State.PositionMeters.Y,
        Arrietty::EyeHeightMeters + State.AltitudeMeters,
        State.HeadingDegrees)));
    const FVector2D HmdForward = Pawn->GetHmdForward();
    const FVector2D BikeForward = Pawn->GetBikeWorldForward();
    const double AlignmentDegrees = FMath::RadiansToDegrees(FMath::Atan2(
        HmdForward.X * BikeForward.Y - HmdForward.Y * BikeForward.X,
        FVector2D::DotProduct(HmdForward, BikeForward)));
    HmdForwardText->SetText(FText::FromString(FString::Printf(
        TEXT("HMD UE Forward  X %+.3f  Y %+.3f\nBike UE Forward X %+.3f  Y %+.3f   Offset %+.1f degrees"),
        HmdForward.X, HmdForward.Y,
        BikeForward.X, BikeForward.Y,
        AlignmentDegrees)));
    RideButtonLabel->SetText(FText::FromString(
        Pawn->IsRideActive() ? TEXT("Riding - Back to Real World to Stop") : TEXT("Start Ride (Numpad 0)")));
    RideStatusText->SetText(FText::FromString(FString::Printf(TEXT("Status: %s"), *LexToString(State.Status))));
    RideMessageText->SetText(FText::FromString(State.Message));
    ControlStatusText->SetText(FText::FromString(FString::Printf(
        TEXT("T2 Control: %s\n%s"), *State.ControlStatus, *State.ControlMessage)));
    const FString HeartRate = State.HeartRateBpm.IsSet()
        ? FString::Printf(TEXT("%u bpm"), State.HeartRateBpm.GetValue())
        : TEXT("-- bpm");
    TelemetryText->SetText(FText::FromString(FString::Printf(
        TEXT("%.2f km/h   %.0f rpm   %d W   HR %s\nHeart sensor: %s"),
        State.SpeedKmh,
        State.CadenceRpm,
        State.PowerWatts,
        *HeartRate,
        *State.HeartRateStatus)));
    DistanceText->SetText(FText::FromString(FString::Printf(
        TEXT("Distance %.1f m   Laps completed %d"), State.DistanceMeters, State.LapsCompleted)));
    LogText->SetText(FText::FromString(FString::Printf(
        TEXT("Log: Saved/arrietty_ride.csv%s"), Pawn->IsRideActive() ? TEXT(" (recording)") : TEXT(""))));
    FlightButtonLabel->SetText(FText::FromString(
        State.bFlightEnabled ? TEXT("Return to Ground (Numpad 7)") : TEXT("Enable Flight (Numpad 7)")));
    FlightStatusText->SetText(FText::FromString(FString::Printf(
        TEXT("Mode: %s   Altitude %.1f m   XY: RIDE SURFACE"),
        State.bFlightEnabled ? TEXT("FLIGHT") : TEXT("GROUND"),
        State.AltitudeMeters)));
    InstrumentButtonLabel->SetText(FText::FromString(
        Pawn->IsInstrumentVisible() ? TEXT("Hide Panel") : TEXT("Show Panel Preview")));
    InstrumentStatusText->SetText(FText::FromString(FString::Printf(
        TEXT("Anchor: %s"), *Pawn->GetInstrumentAnchorStatus())));
    SteeringStatusText->SetText(FText::FromString(FString::Printf(
        TEXT("Status: %s\nRaw %+.1f degrees   Applied %+.1f degrees"),
        State.bSteeringTracking ? TEXT("TRACKING") : TEXT("IDLE / LOST"),
        State.RawSteeringDegrees,
        State.EffectiveSteeringDegrees)));
    const bool bAtTarget = State.AverageFps >= 59.5;
    PerformanceText->SetText(FText::FromString(FString::Printf(
        TEXT("Performance: %.1f FPS (target 60)%s"),
        State.AverageFps,
        bAtTarget ? TEXT("") : TEXT(" - dynamic resolution active"))));
    PerformanceText->SetColorAndOpacity(FSlateColor(
        bAtTarget ? FLinearColor(0.25f, 1.0f, 0.40f) : FLinearColor(1.0f, 0.45f, 0.1f)));
    for (int32 Index = 0; Index < PresetButtons.Num(); ++Index)
    {
        PresetButtons[Index]->SetBackgroundColor(
            State.SelectedPreset == Index + 1
                ? FLinearColor(0.12f, 0.65f, 0.25f)
                : FLinearColor(0.22f, 0.24f, 0.28f));
    }
}

void UArriettyControlWidget::OnToggleVr() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->ToggleVrSession(); }
void UArriettyControlWidget::OnRecenterHmd() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->RecenterHmdToBike(); }
void UArriettyControlWidget::OnExitApplication() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->QuitApplication(); }
void UArriettyControlWidget::OnStartRide() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->StartRide(); }
void UArriettyControlWidget::OnToggleFlight() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->ToggleFlight(); }
void UArriettyControlWidget::OnToggleInstrument() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->ToggleInstrumentPanel(); }
void UArriettyControlWidget::OnPreset1() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SelectControlPreset(1); }
void UArriettyControlWidget::OnPreset2() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SelectControlPreset(2); }
void UArriettyControlWidget::OnPreset3() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SelectControlPreset(3); }
void UArriettyControlWidget::OnPreset4() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SelectControlPreset(4); }
void UArriettyControlWidget::OnPreset5() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SelectControlPreset(5); }
void UArriettyControlWidget::OnPreset6() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SelectControlPreset(6); }
void UArriettyControlWidget::OnPreset7() { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SelectControlPreset(7); }
void UArriettyControlWidget::OnMoveChanged(float Value) { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SetMoveStepMeters(Value); }
void UArriettyControlWidget::OnTurnChanged(float Value) { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SetTurnStepDegrees(Value); }
void UArriettyControlWidget::OnLapChanged(float Value) { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SetLapLengthMeters(Value); }
void UArriettyControlWidget::OnPanelForwardChanged(float Value) { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SetPanelForwardOffsetMeters(Value); }
void UArriettyControlWidget::OnPanelSideChanged(float Value) { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SetPanelSideOffsetMeters(Value); }
void UArriettyControlWidget::OnPanelHeightChanged(float Value) { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SetPanelHeightOffsetMeters(Value); }
void UArriettyControlWidget::OnPanelScaleChanged(float Value) { if (AArriettyPawn* Pawn = FindArriettyPawn()) Pawn->SetPanelScale(Value); }
