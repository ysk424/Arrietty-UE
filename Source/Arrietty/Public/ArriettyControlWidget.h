// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArriettyControlWidget.generated.h"

class AArriettyPawn;
class UButton;
class USpinBox;
class UTextBlock;
class UVerticalBox;

UCLASS()
class ARRIETTY_API UArriettyControlWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    AArriettyPawn* FindArriettyPawn() const;
    UTextBlock* AddText(UVerticalBox* Parent, const FString& Text, int32 Size = 14, const FLinearColor& Color = FLinearColor::White);
    UButton* AddButton(UVerticalBox* Parent, const FString& Label, TObjectPtr<UTextBlock>& OutLabel);
    USpinBox* AddSetting(UVerticalBox* Parent, const FString& Label, double Value, double Min, double Max);
    void Refresh();

    UFUNCTION() void OnToggleVr();
    UFUNCTION() void OnRecenterHmd();
    UFUNCTION() void OnExitApplication();
    UFUNCTION() void OnStartRide();
    UFUNCTION() void OnToggleFlight();
    UFUNCTION() void OnToggleInstrument();
    UFUNCTION() void OnPreset1();
    UFUNCTION() void OnPreset2();
    UFUNCTION() void OnPreset3();
    UFUNCTION() void OnPreset4();
    UFUNCTION() void OnPreset5();
    UFUNCTION() void OnPreset6();
    UFUNCTION() void OnPreset7();
    UFUNCTION() void OnMoveChanged(float Value);
    UFUNCTION() void OnTurnChanged(float Value);
    UFUNCTION() void OnLapChanged(float Value);
    UFUNCTION() void OnPanelForwardChanged(float Value);
    UFUNCTION() void OnPanelSideChanged(float Value);
    UFUNCTION() void OnPanelHeightChanged(float Value);
    UFUNCTION() void OnPanelScaleChanged(float Value);

    UPROPERTY() TObjectPtr<UTextBlock> VrButtonLabel;
    UPROPERTY() TObjectPtr<UTextBlock> VrStatusText;
    UPROPERTY() TObjectPtr<UTextBlock> StartPoseText;
    UPROPERTY() TObjectPtr<UTextBlock> HmdForwardText;
    UPROPERTY() TObjectPtr<UTextBlock> RideButtonLabel;
    UPROPERTY() TObjectPtr<UTextBlock> RideStatusText;
    UPROPERTY() TObjectPtr<UTextBlock> RideMessageText;
    UPROPERTY() TObjectPtr<UTextBlock> ControlStatusText;
    UPROPERTY() TObjectPtr<UTextBlock> TelemetryText;
    UPROPERTY() TObjectPtr<UTextBlock> DistanceText;
    UPROPERTY() TObjectPtr<UTextBlock> LogText;
    UPROPERTY() TObjectPtr<UTextBlock> FlightButtonLabel;
    UPROPERTY() TObjectPtr<UTextBlock> FlightStatusText;
    UPROPERTY() TObjectPtr<UTextBlock> InstrumentButtonLabel;
    UPROPERTY() TObjectPtr<UTextBlock> InstrumentStatusText;
    UPROPERTY() TObjectPtr<UTextBlock> SteeringStatusText;
    UPROPERTY() TObjectPtr<UTextBlock> PerformanceText;
    UPROPERTY() TArray<TObjectPtr<UButton>> PresetButtons;

    double TimeSinceRefresh = 0.0;
};
