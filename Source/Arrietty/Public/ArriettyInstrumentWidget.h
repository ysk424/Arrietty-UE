// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArriettyTypes.h"
#include "ArriettyInstrumentWidget.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS()
class ARRIETTY_API UArriettyInstrumentWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    void SetRideSnapshot(const FArriettyRideSnapshot& Snapshot);

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> SpeedText;

    UPROPERTY()
    TObjectPtr<UTextBlock> HeartRateText;

    UPROPERTY()
    TObjectPtr<UProgressBar> HeartRateBar;

    UPROPERTY()
    TObjectPtr<UTextBlock> ClockText;

    UPROPERTY()
    TObjectPtr<UTextBlock> RideDataText;

    UPROPERTY()
    TObjectPtr<UTextBlock> PositionText;

    UPROPERTY()
    TObjectPtr<UTextBlock> StatusText;

    double LastUpdateSeconds = 0.0;
};
