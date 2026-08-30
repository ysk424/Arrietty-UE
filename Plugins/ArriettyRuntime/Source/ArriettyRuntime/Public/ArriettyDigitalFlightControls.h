// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyTypes.h"

struct FArriettyDigitalFlightControlChange
{
    bool bPitchChanged = false;
    bool bRollChanged = false;
    bool bReset = false;

    bool Any() const { return bPitchChanged || bRollChanged || bReset; }
};

/**
 * Converts Joystick 2 gestures into persistent one-degree flight commands.
 * A gesture is consumed once and must return to the release deadzone before
 * another step can be generated, so holding the stick never repeats.
 */
class ARRIETTYRUNTIME_API FArriettyDigitalFlightControls
{
public:
    void Reset(const FVector2D& CurrentAxes = FVector2D::ZeroVector);
    FArriettyDigitalFlightControlChange ResetCommands(const FVector2D& CurrentAxes);
    FArriettyDigitalFlightControlChange UpdateJoystick(const FVector2D& Axes);
    FArriettyDigitalFlightControlChange StepPitch(int32 Direction);
    FArriettyDigitalFlightControlChange StepRollRight(int32 Direction);

    double GetPitchDegrees() const { return PitchDegrees; }
    double GetRollRightDegrees() const { return RollRightDegrees; }
    double GetElevatorInput() const;
    double GetAileronInput() const;

private:
    static int32 ConsumeGesture(double Value, bool& bArmed);
    void UpdateArming(const FVector2D& Axes);

    double PitchDegrees = 0.0;
    double RollRightDegrees = 0.0;
    bool bPitchArmed = true;
    bool bRollArmed = true;
};
