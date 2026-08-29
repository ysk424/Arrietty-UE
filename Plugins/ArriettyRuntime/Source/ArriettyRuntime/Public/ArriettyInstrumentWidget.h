// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "ArriettyTypes.h"
#include "ArriettyInstrumentWidget.generated.h"

class SArriettyAttitudeIndicator;
class UTextBlock;
class UProgressBar;

/** Native, dependency-free artificial horizon used by the VR instrument panel. */
UCLASS()
class ARRIETTYRUNTIME_API UArriettyAttitudeIndicator : public UWidget
{
    GENERATED_BODY()

public:
    void SetAttitude(double PitchDegrees, double BankDegrees, bool bWarning);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    TSharedPtr<SArriettyAttitudeIndicator> SlateIndicator;
    double PitchDegrees = 0.0;
    double BankDegrees = 0.0;
    bool bWarning = false;
};

UCLASS()
class ARRIETTYRUNTIME_API UArriettyInstrumentWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetRideSnapshot(const FArriettyRideSnapshot& Snapshot);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> SpeedText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SpeedCueText;

    UPROPERTY()
    TObjectPtr<UArriettyAttitudeIndicator> AttitudeIndicator;

    UPROPERTY()
    TObjectPtr<UTextBlock> PowerText;

    UPROPERTY()
    TObjectPtr<UProgressBar> HeartRateBar;

    UPROPERTY()
    TObjectPtr<UTextBlock> ClockText;

    UPROPERTY()
    TObjectPtr<UTextBlock> FlightDataText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SensorText;

    UPROPERTY()
    TObjectPtr<UTextBlock> PositionText;

    UPROPERTY()
    TObjectPtr<UTextBlock> StatusText;

    double LastUpdateSeconds = 0.0;
};
