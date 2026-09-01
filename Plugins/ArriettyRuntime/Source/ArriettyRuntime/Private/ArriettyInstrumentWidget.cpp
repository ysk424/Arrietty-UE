// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyInstrumentWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

namespace
{
const FLinearColor InstrumentGreen(0.05f, 0.95f, 0.45f, 1.0f);
const FLinearColor InstrumentDimGreen(0.03f, 0.35f, 0.18f, 1.0f);
const FLinearColor InstrumentOrange(1.0f, 0.58f, 0.02f, 1.0f);
const FLinearColor InstrumentRed(1.0f, 0.04f, 0.08f, 1.0f);
const FLinearColor InstrumentBlack(0.002f, 0.008f, 0.006f, 1.0f);

UTextBlock* MakeInstrumentText(
    UWidgetTree* Tree,
    UPanelWidget* Parent,
    const FString& InitialText,
    int32 FontSize,
    const FLinearColor& Color,
    ETextJustify::Type Justification = ETextJustify::Left)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(InitialText));
    Text->SetColorAndOpacity(FSlateColor(Color));
    Text->SetJustification(Justification);
    Text->SetShadowOffset(FVector2D(1.0, 1.0));
    Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = FontSize;
    Font.OutlineSettings.OutlineSize = FontSize >= 30 ? 1 : 0;
    Font.OutlineSettings.OutlineColor = FLinearColor::Black;
    Text->SetFont(Font);
    Parent->AddChild(Text);
    return Text;
}

UBorder* MakePanel(UWidgetTree* Tree, UPanelWidget* Parent, const FMargin& Padding)
{
    UBorder* Panel = Tree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(InstrumentBlack);
    Panel->SetPadding(Padding);
    Parent->AddChild(Panel);
    return Panel;
}
}

class SArriettyAttitudeIndicator : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SArriettyAttitudeIndicator) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        SetCanTick(false);
    }

    void SetAttitude(double InPitchDegrees, double InBankDegrees, bool bInWarning)
    {
        PitchDegrees = FMath::Clamp(InPitchDegrees, -30.0, 30.0);
        BankDegrees = FMath::Clamp(InBankDegrees, -45.0, 45.0);
        bWarning = bInWarning;
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
    {
        return FVector2D(300.0, 255.0);
    }

    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        const FVector2D Center = Size * 0.5;
        const FLinearColor ActiveColor = bWarning ? InstrumentRed : InstrumentGreen;
        const float BankRadians = FMath::DegreesToRadians(static_cast<float>(BankDegrees));
        const FVector2D Across(FMath::Cos(BankRadians), FMath::Sin(BankRadians));
        const FVector2D Up(-Across.Y, Across.X);
        const FVector2D HorizonCenter = Center + Up * (PitchDegrees * 4.0);
        const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

        auto DrawLine = [&](int32 Layer, const FVector2D& A, const FVector2D& B,
                            const FLinearColor& Color, float Thickness)
        {
            TArray<FVector2f> Points;
            Points.Reserve(2);
            Points.Add(FVector2f(A));
            Points.Add(FVector2f(B));
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                Layer,
                PaintGeometry,
                Points,
                ESlateDrawEffect::None,
                Color,
                true,
                Thickness);
        };

        DrawLine(LayerId, FVector2D(1.0, 1.0), FVector2D(Size.X - 1.0, 1.0),
            InstrumentDimGreen, 2.0f);
        DrawLine(LayerId, FVector2D(Size.X - 1.0, 1.0), FVector2D(Size.X - 1.0, Size.Y - 1.0),
            InstrumentDimGreen, 2.0f);
        DrawLine(LayerId, FVector2D(Size.X - 1.0, Size.Y - 1.0), FVector2D(1.0, Size.Y - 1.0),
            InstrumentDimGreen, 2.0f);
        DrawLine(LayerId, FVector2D(1.0, Size.Y - 1.0), FVector2D(1.0, 1.0),
            InstrumentDimGreen, 2.0f);

        DrawLine(LayerId + 1, HorizonCenter - Across * Size.X, HorizonCenter + Across * Size.X,
            ActiveColor, 4.0f);
        for (int32 Degrees = -20; Degrees <= 20; Degrees += 5)
        {
            if (Degrees == 0)
            {
                continue;
            }
            const FVector2D LadderCenter = HorizonCenter - Up * (Degrees * 4.0);
            const double HalfWidth = Degrees % 10 == 0 ? 50.0 : 28.0;
            DrawLine(LayerId + 1,
                LadderCenter - Across * HalfWidth,
                LadderCenter + Across * HalfWidth,
                InstrumentDimGreen,
                Degrees % 10 == 0 ? 2.0f : 1.0f);
        }

        const FVector2D TopCenter(Center.X, 10.0);
        DrawLine(LayerId + 2, TopCenter + FVector2D(-9.0, 12.0), TopCenter,
            ActiveColor, 3.0f);
        DrawLine(LayerId + 2, TopCenter, TopCenter + FVector2D(9.0, 12.0),
            ActiveColor, 3.0f);
        DrawLine(LayerId + 2, Center + FVector2D(-65.0, 0.0), Center + FVector2D(-18.0, 0.0),
            ActiveColor, 5.0f);
        DrawLine(LayerId + 2, Center + FVector2D(18.0, 0.0), Center + FVector2D(65.0, 0.0),
            ActiveColor, 5.0f);
        DrawLine(LayerId + 2, Center + FVector2D(-18.0, 0.0), Center + FVector2D(0.0, 10.0),
            ActiveColor, 5.0f);
        DrawLine(LayerId + 2, Center + FVector2D(0.0, 10.0), Center + FVector2D(18.0, 0.0),
            ActiveColor, 5.0f);
        return LayerId + 2;
    }

private:
    double PitchDegrees = 0.0;
    double BankDegrees = 0.0;
    bool bWarning = false;
};

void UArriettyAttitudeIndicator::SetAttitude(
    double InPitchDegrees,
    double InBankDegrees,
    bool bInWarning)
{
    PitchDegrees = InPitchDegrees;
    BankDegrees = InBankDegrees;
    bWarning = bInWarning;
    if (SlateIndicator)
    {
        SlateIndicator->SetAttitude(PitchDegrees, BankDegrees, bWarning);
    }
}

TSharedRef<SWidget> UArriettyAttitudeIndicator::RebuildWidget()
{
    SlateIndicator = SNew(SArriettyAttitudeIndicator);
    SlateIndicator->SetAttitude(PitchDegrees, BankDegrees, bWarning);
    return SlateIndicator.ToSharedRef();
}

void UArriettyAttitudeIndicator::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    SlateIndicator.Reset();
}

TSharedRef<SWidget> UArriettyInstrumentWidget::RebuildWidget()
{
    Initialize();
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return Super::RebuildWidget();
    }

    UBorder* OuterBezel = WidgetTree->ConstructWidget<UBorder>();
    OuterBezel->SetBrushColor(InstrumentGreen);
    OuterBezel->SetPadding(FMargin(7.0f));
    WidgetTree->RootWidget = OuterBezel;

    UBorder* Screen = WidgetTree->ConstructWidget<UBorder>();
    Screen->SetBrushColor(InstrumentBlack);
    Screen->SetPadding(FMargin(14.0f, 10.0f));
    OuterBezel->SetContent(Screen);

    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
    Screen->SetContent(Root);

    UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
    Root->AddChildToVerticalBox(Header);
    UTextBlock* Title = MakeInstrumentText(
        WidgetTree, Header, TEXT("ARRIETTY // WINGS OVER THE EARTH"), 20, InstrumentGreen);
    ClockText = MakeInstrumentText(
        WidgetTree, Header, TEXT("T+00:00:00"), 20, InstrumentOrange, ETextJustify::Right);
    if (UHorizontalBoxSlot* TitleSlot = Cast<UHorizontalBoxSlot>(Title->Slot))
    {
        TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    if (UHorizontalBoxSlot* ClockSlot = Cast<UHorizontalBoxSlot>(ClockText->Slot))
    {
        ClockSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    if (UVerticalBoxSlot* MainSlot = Root->AddChildToVerticalBox(MainRow))
    {
        MainSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        MainSlot->SetPadding(FMargin(0.0f, 7.0f));
    }

    USizeBox* SpeedSize = WidgetTree->ConstructWidget<USizeBox>();
    SpeedSize->SetWidthOverride(250.0f);
    MainRow->AddChildToHorizontalBox(SpeedSize);
    UBorder* SpeedPanel = WidgetTree->ConstructWidget<UBorder>();
    SpeedPanel->SetBrushColor(InstrumentBlack);
    SpeedPanel->SetPadding(FMargin(4.0f));
    SpeedSize->SetContent(SpeedPanel);
    UVerticalBox* SpeedColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    SpeedPanel->SetContent(SpeedColumn);
    MakeInstrumentText(
        WidgetTree, SpeedColumn, TEXT("AIRSPEED"), 18, InstrumentDimGreen, ETextJustify::Center);
    SpeedText = MakeInstrumentText(
        WidgetTree, SpeedColumn, TEXT("0.0"), 82, InstrumentGreen, ETextJustify::Center);
    MakeInstrumentText(
        WidgetTree, SpeedColumn, TEXT("km/h"), 24, InstrumentGreen, ETextJustify::Center);
    SpeedCueText = MakeInstrumentText(
        WidgetTree, SpeedColumn, TEXT("GROUND ROLL"), 19, InstrumentOrange, ETextJustify::Center);
    PowerText = MakeInstrumentText(
        WidgetTree,
        SpeedColumn,
        TEXT("RIDER    0 W\nPROP     0 W\nTUNE OFF - PRESS J1 SW\nPTT READY"),
        18,
        InstrumentGreen);

    USizeBox* AttitudeSize = WidgetTree->ConstructWidget<USizeBox>();
    AttitudeSize->SetWidthOverride(310.0f);
    AttitudeSize->SetHeightOverride(275.0f);
    if (UHorizontalBoxSlot* AttitudeSlot = MainRow->AddChildToHorizontalBox(AttitudeSize))
    {
        AttitudeSlot->SetPadding(FMargin(9.0f, 0.0f));
        AttitudeSlot->SetVerticalAlignment(VAlign_Center);
    }
    AttitudeIndicator = WidgetTree->ConstructWidget<UArriettyAttitudeIndicator>();
    AttitudeIndicator->SetClipping(EWidgetClipping::ClipToBounds);
    AttitudeSize->SetContent(AttitudeIndicator);

    UBorder* DataPanel = MakePanel(WidgetTree, MainRow, FMargin(5.0f));
    if (UHorizontalBoxSlot* DataSlot = Cast<UHorizontalBoxSlot>(DataPanel->Slot))
    {
        DataSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    FlightDataText = MakeInstrumentText(
        WidgetTree,
        DataPanel,
        TEXT("STATE  ON GROUND\nALT AGL   0.0 m\nV/S      +0.0 m/s\nHDG        000\nPITCH    +0.0\nBANK     +0.0\nCMD R    +0\nCMD P    +0\nFPA      +0.0\nAOA      +0.0"),
        20,
        InstrumentGreen);

    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
    Root->AddChildToVerticalBox(Footer);
    SensorText = MakeInstrumentText(
        WidgetTree, Footer, TEXT("CAD   0 rpm  HR ---  DIST 0 m"), 19, InstrumentGreen);
    PositionText = MakeInstrumentText(
        WidgetTree, Footer, TEXT("X +0.0  Y +0.0 m"), 17, InstrumentDimGreen, ETextJustify::Right);
    if (UHorizontalBoxSlot* SensorSlot = Cast<UHorizontalBoxSlot>(SensorText->Slot))
    {
        SensorSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    if (UHorizontalBoxSlot* PositionSlot = Cast<UHorizontalBoxSlot>(PositionText->Slot))
    {
        PositionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    USizeBox* HeartRateBarSize = WidgetTree->ConstructWidget<USizeBox>();
    HeartRateBarSize->SetHeightOverride(7.0f);
    Root->AddChildToVerticalBox(HeartRateBarSize);
    HeartRateBar = WidgetTree->ConstructWidget<UProgressBar>();
    HeartRateBar->SetPercent(0.0f);
    HeartRateBar->SetFillColorAndOpacity(InstrumentOrange);
    HeartRateBarSize->SetContent(HeartRateBar);

    StatusText = MakeInstrumentText(
        WidgetTree, Root, TEXT("SYSTEM READY"), 18, InstrumentOrange, ETextJustify::Center);
    StatusText->SetAutoWrapText(true);

    // Native widgets must populate WidgetTree before the base class caches it.
    return Super::RebuildWidget();
}

void UArriettyInstrumentWidget::SetRideSnapshot(const FArriettyRideSnapshot& Snapshot)
{
    const double Now = FPlatformTime::Seconds();
    if (Now - LastUpdateSeconds < 0.10 || !SpeedText)
    {
        return;
    }
    LastUpdateSeconds = Now;

    const bool bFlightWarning = Snapshot.bAircraftStalled || Snapshot.bAircraftOverspeed;
    const FLinearColor SpeedColor = bFlightWarning ? InstrumentRed : InstrumentGreen;
    SpeedText->SetText(FText::FromString(FString::Printf(TEXT("%4.1f"), Snapshot.SpeedKmh)));
    SpeedText->SetColorAndOpacity(FSlateColor(SpeedColor));

    FString SpeedCue = TEXT("GROUND SPEED");
    FLinearColor SpeedCueColor = InstrumentOrange;
    if (Snapshot.bFlightEnabled)
    {
        const double DisplayTakeoffSpeed = Arrietty::TakeoffSpeedKmh;
        const double DisplayStallSpeed = Arrietty::FlightStallSpeedKmh;
        const double DisplayRecoverySpeed = Arrietty::FlightStallRecoverySpeedKmh;
        if (!Snapshot.bAircraftAirborne)
        {
            SpeedCue = Snapshot.SpeedKmh >= DisplayTakeoffSpeed
                ? TEXT("ROTATE")
                : FString::Printf(TEXT("TAKEOFF %.0f"), DisplayTakeoffSpeed);
        }
        else if (Snapshot.bAircraftOverspeed)
        {
            SpeedCue = TEXT("OVERSPEED");
            SpeedCueColor = InstrumentRed;
        }
        else if (Snapshot.bAircraftStalled ||
            Snapshot.SpeedKmh < DisplayStallSpeed)
        {
            SpeedCue = TEXT("STALL");
            SpeedCueColor = InstrumentRed;
        }
        else if (Snapshot.SpeedKmh < DisplayRecoverySpeed)
        {
            SpeedCue = TEXT("RECOVERY BAND");
        }
        else
        {
            SpeedCue = FString::Printf(
                TEXT("CONTROL x%.2f"), Snapshot.FlightControlAuthority);
            SpeedCueColor = InstrumentGreen;
        }
    }
    SpeedCueText->SetText(FText::FromString(SpeedCue));
    SpeedCueText->SetColorAndOpacity(FSlateColor(SpeedCueColor));

    PowerText->SetText(FText::FromString(FString::Printf(
        TEXT("RIDER %4d W\nPROP  %4.0f W\n%s\n%s"),
        Snapshot.PowerWatts,
        Snapshot.PropulsionPowerWatts,
        *Snapshot.FlightTuningStatus,
        *Snapshot.VoiceStatus)));
    PowerText->SetColorAndOpacity(FSlateColor(
        Snapshot.bPushToTalkHeld ? InstrumentOrange : InstrumentGreen));

    if (AttitudeIndicator)
    {
        AttitudeIndicator->SetAttitude(
            Snapshot.PitchDegrees,
            Snapshot.BankDegrees,
            bFlightWarning);
    }

    const int32 HeadingDegrees = FMath::RoundToInt(
        FMath::Fmod(Snapshot.HeadingDegrees + 360.0, 360.0)) % 360;
    FlightDataText->SetText(FText::FromString(FString::Printf(
        TEXT("STATE  %s\nALT AGL %6.1f m\nV/S     %+6.1f m/s\nHDG        %03d\nPITCH   %+6.1f\nBANK    %+6.1f\nCMD R   %+6.0f\nCMD P   %+6.0f\nFPA     %+6.1f\nAOA     %+6.1f"),
        Snapshot.bAircraftAirborne ? TEXT("AIRBORNE") : TEXT("ON GROUND"),
        Snapshot.AltitudeMeters,
        Snapshot.VerticalSpeedMetersPerSecond,
        HeadingDegrees,
        Snapshot.PitchDegrees,
        Snapshot.BankDegrees,
        Snapshot.CommandedRollRightDegrees,
        Snapshot.CommandedPitchDegrees,
        Snapshot.FlightPathAngleDegrees,
        Snapshot.AngleOfAttackDegrees)));
    FlightDataText->SetColorAndOpacity(FSlateColor(
        bFlightWarning ? InstrumentRed : InstrumentGreen));

    const FString Distance = Snapshot.DistanceMeters < 1000.0
        ? FString::Printf(TEXT("%.0f m"), Snapshot.DistanceMeters)
        : FString::Printf(TEXT("%.2f km"), Snapshot.DistanceMeters / 1000.0);
    const FString HeartRate = Snapshot.HeartRateBpm.IsSet()
        ? FString::Printf(TEXT("%u"), Snapshot.HeartRateBpm.GetValue())
        : TEXT("---");
    SensorText->SetText(FText::FromString(FString::Printf(
        TEXT("CAD %3.0f rpm  HR %s  DIST %s"),
        Snapshot.CadenceRpm,
        *HeartRate,
        *Distance)));
    if (Snapshot.HeartRateBpm.IsSet())
    {
        const uint16 HeartRateValue = Snapshot.HeartRateBpm.GetValue();
        HeartRateBar->SetPercent(FMath::Clamp(
            (static_cast<float>(HeartRateValue) - 40.0f) / 160.0f, 0.0f, 1.0f));
        HeartRateBar->SetFillColorAndOpacity(
            HeartRateValue >= 170 ? InstrumentRed : InstrumentOrange);
    }
    else
    {
        HeartRateBar->SetPercent(0.0f);
    }

    ClockText->SetText(FText::FromString(FormatElapsedTime(Snapshot.ElapsedSeconds)));
    const FString Position = Snapshot.bGeospatialNavigation
        ? FString::Printf(
            TEXT("LON %.6f  LAT %.6f  ELLIP H %.1f m"),
            Snapshot.LongitudeDegrees,
            Snapshot.LatitudeDegrees,
            Snapshot.EllipsoidHeightMeters)
        : FString::Printf(
            TEXT("X %+.1f  Y %+.1f m"),
            Snapshot.PositionMeters.X,
            Snapshot.PositionMeters.Y);
    PositionText->SetText(FText::FromString(Position));

    StatusText->SetText(FText::FromString(Snapshot.Message));
    const bool bStatusWarning = Snapshot.Status == EArriettyRideStatus::Error ||
        bFlightWarning ||
        Snapshot.Message.StartsWith(TEXT("Ride paused;"));
    StatusText->SetColorAndOpacity(FSlateColor(
        bStatusWarning ? InstrumentRed : InstrumentOrange));
}

FString UArriettyInstrumentWidget::FormatElapsedTime(double ElapsedSeconds)
{
    const int64 TotalSeconds = static_cast<int64>(FMath::Max(0.0, ElapsedSeconds));
    const int64 Hours = TotalSeconds / 3600;
    const int64 Minutes = (TotalSeconds / 60) % 60;
    const int64 Seconds = TotalSeconds % 60;
    return FString::Printf(
        TEXT("T+%02lld:%02lld:%02lld"),
        static_cast<long long>(Hours),
        static_cast<long long>(Minutes),
        static_cast<long long>(Seconds));
}
