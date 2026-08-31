// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyTypes.h"

struct FArriettyFlightTuningChange
{
    bool bEntered = false;
    bool bValueChanged = false;
    bool bAdvanced = false;
    bool bCompleted = false;

    bool Any() const
    {
        return bEntered || bValueChanged || bAdvanced || bCompleted;
    }
};

class FArriettyFlightTuningControls
{
public:
    void Reset(const FVector2D& CurrentAxes = FVector2D::ZeroVector);
    FArriettyFlightTuningChange PressSwitch(
        const FVector2D& CurrentAxes = FVector2D::ZeroVector);
    FArriettyFlightTuningChange UpdateJoystick(const FVector2D& Axes);

    bool IsActive() const { return bActive; }
    EArriettyFlightTuningParameter GetParameter() const { return Parameter; }
    int32 GetParameterNumber() const;
    int32 GetParameterCount() const;
    double GetCurrentValue() const;
    const FArriettyFlightTuningValues& GetValues() const { return Values; }
    FString GetParameterLabel() const;
    FString GetCompactStatus() const;

private:
    bool StepCurrentValue(int32 Direction);
    FString FormatCurrentValue() const;

    FArriettyFlightTuningValues Values;
    EArriettyFlightTuningParameter Parameter =
        EArriettyFlightTuningParameter::TestPropulsionPower;
    bool bActive = false;
    bool bHorizontalArmed = true;
};
