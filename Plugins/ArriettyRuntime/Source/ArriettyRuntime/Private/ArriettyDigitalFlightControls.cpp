// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyDigitalFlightControls.h"

namespace
{
constexpr double GestureTrigger = 0.45;
constexpr double GestureRelease = 0.20;
}

void FArriettyDigitalFlightControls::Reset(const FVector2D& CurrentAxes)
{
    PitchDegrees = 0.0;
    RollRightDegrees = 0.0;
    UpdateArming(CurrentAxes);
}

FArriettyDigitalFlightControlChange FArriettyDigitalFlightControls::ResetCommands(
    const FVector2D& CurrentAxes)
{
    const bool bHadPitch = !FMath::IsNearlyZero(PitchDegrees);
    const bool bHadRoll = !FMath::IsNearlyZero(RollRightDegrees);
    Reset(CurrentAxes);
    return {bHadPitch, bHadRoll, true};
}

FArriettyDigitalFlightControlChange FArriettyDigitalFlightControls::UpdateJoystick(
    const FVector2D& Axes)
{
    FArriettyDigitalFlightControlChange Change;
    const int32 PitchStep = ConsumeGesture(Axes.X, bPitchArmed);
    // The installed Joystick 2 reports negative Y when moved to the rider's
    // right, so invert it to keep the public command convention "right = +".
    const int32 RollRightStep = -ConsumeGesture(Axes.Y, bRollArmed);
    if (PitchStep != 0)
    {
        Change = StepPitch(PitchStep);
    }
    if (RollRightStep != 0)
    {
        const FArriettyDigitalFlightControlChange RollChange = StepRollRight(RollRightStep);
        Change.bRollChanged = RollChange.bRollChanged;
    }
    return Change;
}

FArriettyDigitalFlightControlChange FArriettyDigitalFlightControls::StepPitch(int32 Direction)
{
    const double Previous = PitchDegrees;
    PitchDegrees = FMath::Clamp(
        PitchDegrees + FMath::Sign(static_cast<double>(Direction)) * Arrietty::FlightControlStepDegrees,
        -Arrietty::FlightMaxPitchDegrees,
        Arrietty::FlightMaxPitchDegrees);
    return {!FMath::IsNearlyEqual(Previous, PitchDegrees), false, false};
}

FArriettyDigitalFlightControlChange FArriettyDigitalFlightControls::StepRollRight(int32 Direction)
{
    const double Previous = RollRightDegrees;
    RollRightDegrees = FMath::Clamp(
        RollRightDegrees + FMath::Sign(static_cast<double>(Direction)) * Arrietty::FlightControlStepDegrees,
        -Arrietty::FlightMaxBankDegrees,
        Arrietty::FlightMaxBankDegrees);
    return {false, !FMath::IsNearlyEqual(Previous, RollRightDegrees), false};
}

int32 FArriettyDigitalFlightControls::ConsumeGesture(double Value, bool& bArmed)
{
    if (!bArmed)
    {
        if (FMath::Abs(Value) <= GestureRelease)
        {
            bArmed = true;
        }
        return 0;
    }
    if (FMath::Abs(Value) < GestureTrigger)
    {
        return 0;
    }
    bArmed = false;
    return Value > 0.0 ? 1 : -1;
}

void FArriettyDigitalFlightControls::UpdateArming(const FVector2D& Axes)
{
    bPitchArmed = FMath::Abs(Axes.X) <= GestureRelease;
    bRollArmed = FMath::Abs(Axes.Y) <= GestureRelease;
}
