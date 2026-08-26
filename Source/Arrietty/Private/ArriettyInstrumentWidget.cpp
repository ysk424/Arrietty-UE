// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ArriettyInstrumentWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"

namespace
{
const FLinearColor InstrumentGreen(0.04f, 1.0f, 0.12f, 1.0f);
const FLinearColor InstrumentOrange(1.0f, 0.25f, 0.01f, 1.0f);

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
    Font.OutlineSettings.OutlineSize = 1;
    Font.OutlineSettings.OutlineColor = FLinearColor::Black;
    Text->SetFont(Font);
    Parent->AddChild(Text);
    return Text;
}
}

void UArriettyInstrumentWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
    WidgetTree->RootWidget = Root;

    UHorizontalBox* SpeedRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Root->AddChildToVerticalBox(SpeedRow);
    SpeedText = MakeInstrumentText(
        WidgetTree, SpeedRow, TEXT("0.0"), 96, InstrumentGreen, ETextJustify::Center);
    UTextBlock* UnitText = MakeInstrumentText(
        WidgetTree, SpeedRow, TEXT(" km/h"), 34, InstrumentGreen, ETextJustify::Right);
    if (UHorizontalBoxSlot* SpeedSlot = Cast<UHorizontalBoxSlot>(SpeedText->Slot))
    {
        SpeedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SpeedSlot->SetHorizontalAlignment(HAlign_Center);
    }
    if (UHorizontalBoxSlot* UnitSlot = Cast<UHorizontalBoxSlot>(UnitText->Slot))
    {
        UnitSlot->SetVerticalAlignment(VAlign_Bottom);
        UnitSlot->SetPadding(FMargin(0.0f, 0.0f, 20.0f, 8.0f));
    }

    UHorizontalBox* SensorRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Root->AddChildToVerticalBox(SensorRow);
    HeartRateText = MakeInstrumentText(WidgetTree, SensorRow, TEXT("HR  -- bpm"), 35, InstrumentOrange);
    ClockText = MakeInstrumentText(WidgetTree, SensorRow, TEXT("00:00:00"), 35, InstrumentOrange, ETextJustify::Right);
    if (UHorizontalBoxSlot* HeartSlot = Cast<UHorizontalBoxSlot>(HeartRateText->Slot))
    {
        HeartSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    if (UHorizontalBoxSlot* TimeSlot = Cast<UHorizontalBoxSlot>(ClockText->Slot))
    {
        TimeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    RideDataText = MakeInstrumentText(
        WidgetTree,
        Root,
        TEXT("CAD   0 rpm    PWR    0 W\nDIST    0 m     LAP    0\nALT   0.0 m     GROUND P5"),
        29,
        InstrumentGreen);
    PositionText = MakeInstrumentText(
        WidgetTree, Root, TEXT("X +0.0   Y +0.0 m"), 25, InstrumentGreen);
}

void UArriettyInstrumentWidget::SetRideSnapshot(const FArriettyRideSnapshot& Snapshot)
{
    const double Now = FPlatformTime::Seconds();
    if (Now - LastUpdateSeconds < 0.10 || !SpeedText)
    {
        return;
    }
    LastUpdateSeconds = Now;
    SpeedText->SetText(FText::FromString(FString::Printf(TEXT("%4.1f"), Snapshot.SpeedKmh)));
    HeartRateText->SetText(FText::FromString(TEXT("HR  -- bpm")));
    ClockText->SetText(FText::FromString(FDateTime::Now().ToString(TEXT("%H:%M:%S"))));
    const FString Distance = Snapshot.DistanceMeters < 1000.0
        ? FString::Printf(TEXT("%5.0f m"), Snapshot.DistanceMeters)
        : FString::Printf(TEXT("%5.2f km"), Snapshot.DistanceMeters / 1000.0);
    const int32 Preset = Snapshot.AppliedPreset.IsSet()
        ? Snapshot.AppliedPreset.GetValue()
        : Snapshot.SelectedPreset;
    RideDataText->SetText(FText::FromString(FString::Printf(
        TEXT("CAD %3.0f rpm    PWR %4d W\nDIST %s  LAP %4d\nALT %5.1f m   %s P%d"),
        Snapshot.CadenceRpm,
        Snapshot.PowerWatts,
        *Distance,
        Snapshot.LapsCompleted,
        Snapshot.AltitudeMeters,
        Snapshot.bFlightEnabled ? TEXT("FLIGHT") : TEXT("GROUND"),
        Preset)));
    PositionText->SetText(FText::FromString(FString::Printf(
        TEXT("X %+.1f   Y %+.1f m"), Snapshot.PositionMeters.X, Snapshot.PositionMeters.Y)));
}
