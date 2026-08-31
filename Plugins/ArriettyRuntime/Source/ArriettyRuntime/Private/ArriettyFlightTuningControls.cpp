// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyFlightTuningControls.h"

namespace
{
constexpr double GestureTrigger = 0.45;
constexpr double GestureRelease = 0.20;
}

void FArriettyFlightTuningControls::Reset(const FVector2D& CurrentAxes)
{
    bActive = false;
    Parameter = EArriettyFlightTuningParameter::TestPropulsionPower;
    bHorizontalArmed = FMath::Abs(CurrentAxes.X) <= GestureRelease;
}

FArriettyFlightTuningChange FArriettyFlightTuningControls::PressSwitch(
    const FVector2D& CurrentAxes)
{
    bHorizontalArmed = FMath::Abs(CurrentAxes.X) <= GestureRelease;
    if (!bActive)
    {
        bActive = true;
        Parameter = EArriettyFlightTuningParameter::TestPropulsionPower;
        return {true, false, false, false};
    }

    const int32 Next = static_cast<int32>(Parameter) + 1;
    if (Next < static_cast<int32>(EArriettyFlightTuningParameter::Count))
    {
        Parameter = static_cast<EArriettyFlightTuningParameter>(Next);
        return {false, false, true, false};
    }

    bActive = false;
    Parameter = EArriettyFlightTuningParameter::TestPropulsionPower;
    return {false, false, false, true};
}

FArriettyFlightTuningChange FArriettyFlightTuningControls::UpdateJoystick(
    const FVector2D& Axes)
{
    if (!bActive)
    {
        return {};
    }
    if (!bHorizontalArmed)
    {
        if (FMath::Abs(Axes.X) <= GestureRelease)
        {
            bHorizontalArmed = true;
        }
        return {};
    }
    if (FMath::Abs(Axes.X) < GestureTrigger)
    {
        return {};
    }

    bHorizontalArmed = false;
    const bool bChanged = StepCurrentValue(Axes.X > 0.0 ? 1 : -1);
    return {false, bChanged, false, false};
}

int32 FArriettyFlightTuningControls::GetParameterNumber() const
{
    return static_cast<int32>(Parameter) + 1;
}

int32 FArriettyFlightTuningControls::GetParameterCount() const
{
    return static_cast<int32>(EArriettyFlightTuningParameter::Count);
}

double FArriettyFlightTuningControls::GetCurrentValue() const
{
    switch (Parameter)
    {
    case EArriettyFlightTuningParameter::TestPropulsionPower:
        return Values.TestPropulsionPowerWatts;
    case EArriettyFlightTuningParameter::AirspeedMultiplier:
        return Values.AirspeedMultiplier;
    case EArriettyFlightTuningParameter::PositiveClimbMultiplier:
        return Values.PositiveClimbMultiplier;
    case EArriettyFlightTuningParameter::PitchResponseRate:
        return Values.PitchRateDegreesPerSecond;
    case EArriettyFlightTuningParameter::ElevatorVerticalSpeed:
        return Values.MaxElevatorVerticalSpeedMps;
    case EArriettyFlightTuningParameter::BankResponseRate:
        return Values.BankRateDegreesPerSecond;
    default:
        return 0.0;
    }
}

FString FArriettyFlightTuningControls::GetParameterLabel() const
{
    switch (Parameter)
    {
    case EArriettyFlightTuningParameter::TestPropulsionPower:
        return TEXT("TEST PROP");
    case EArriettyFlightTuningParameter::AirspeedMultiplier:
        return TEXT("SPEED");
    case EArriettyFlightTuningParameter::PositiveClimbMultiplier:
        return TEXT("CLIMB");
    case EArriettyFlightTuningParameter::PitchResponseRate:
        return TEXT("PITCH RATE");
    case EArriettyFlightTuningParameter::ElevatorVerticalSpeed:
        return TEXT("PITCH V/S");
    case EArriettyFlightTuningParameter::BankResponseRate:
        return TEXT("ROLL RATE");
    default:
        return TEXT("UNKNOWN");
    }
}

FString FArriettyFlightTuningControls::GetCompactStatus() const
{
    if (!bActive)
    {
        return TEXT("TUNE OFF - PRESS J1 SW");
    }
    return FString::Printf(
        TEXT("TUNE %d/%d %s %s"),
        GetParameterNumber(),
        GetParameterCount(),
        *GetParameterLabel(),
        *FormatCurrentValue());
}

bool FArriettyFlightTuningControls::StepCurrentValue(int32 Direction)
{
    const double Sign = Direction >= 0 ? 1.0 : -1.0;
    const double Previous = GetCurrentValue();
    switch (Parameter)
    {
    case EArriettyFlightTuningParameter::TestPropulsionPower:
        Values.TestPropulsionPowerWatts = FMath::Clamp(
            Values.TestPropulsionPowerWatts +
                Sign * Arrietty::FlightTestPropulsionPowerStepWatts,
            Arrietty::FlightMinTestPropulsionPowerWatts,
            Arrietty::FlightMaxTestPropulsionPowerWatts);
        break;
    case EArriettyFlightTuningParameter::AirspeedMultiplier:
        Values.AirspeedMultiplier = FMath::Clamp(
            Values.AirspeedMultiplier +
                Sign * Arrietty::FlightAirspeedMultiplierStep,
            Arrietty::FlightMinAirspeedMultiplier,
            Arrietty::FlightMaxAirspeedMultiplier);
        break;
    case EArriettyFlightTuningParameter::PositiveClimbMultiplier:
        Values.PositiveClimbMultiplier = FMath::Clamp(
            Values.PositiveClimbMultiplier +
                Sign * Arrietty::FlightPositiveClimbMultiplierStep,
            Arrietty::FlightMinPositiveClimbMultiplier,
            Arrietty::FlightMaxPositiveClimbMultiplier);
        break;
    case EArriettyFlightTuningParameter::PitchResponseRate:
        Values.PitchRateDegreesPerSecond = FMath::Clamp(
            Values.PitchRateDegreesPerSecond +
                Sign * Arrietty::FlightPitchRateStepDegreesPerSecond,
            Arrietty::FlightMinPitchRateDegreesPerSecond,
            Arrietty::FlightMaxPitchRateTuningDegreesPerSecond);
        break;
    case EArriettyFlightTuningParameter::ElevatorVerticalSpeed:
        Values.MaxElevatorVerticalSpeedMps = FMath::Clamp(
            Values.MaxElevatorVerticalSpeedMps +
                Sign * Arrietty::FlightVerticalSpeedStepMps,
            Arrietty::FlightMinElevatorVerticalSpeedMps,
            Arrietty::FlightMaxElevatorVerticalSpeedTuningMps);
        break;
    case EArriettyFlightTuningParameter::BankResponseRate:
        Values.BankRateDegreesPerSecond = FMath::Clamp(
            Values.BankRateDegreesPerSecond +
                Sign * Arrietty::FlightBankRateStepDegreesPerSecond,
            Arrietty::FlightMinBankRateDegreesPerSecond,
            Arrietty::FlightMaxBankRateTuningDegreesPerSecond);
        break;
    default:
        return false;
    }
    return !FMath::IsNearlyEqual(Previous, GetCurrentValue());
}

FString FArriettyFlightTuningControls::FormatCurrentValue() const
{
    switch (Parameter)
    {
    case EArriettyFlightTuningParameter::TestPropulsionPower:
        return FString::Printf(TEXT("%.0f W"), GetCurrentValue());
    case EArriettyFlightTuningParameter::AirspeedMultiplier:
        return FString::Printf(TEXT("x%.1f"), GetCurrentValue());
    case EArriettyFlightTuningParameter::PositiveClimbMultiplier:
        return FString::Printf(TEXT("x%.0f"), GetCurrentValue());
    case EArriettyFlightTuningParameter::PitchResponseRate:
    case EArriettyFlightTuningParameter::BankResponseRate:
        return FString::Printf(TEXT("%.0f DEG/S"), GetCurrentValue());
    case EArriettyFlightTuningParameter::ElevatorVerticalSpeed:
        return FString::Printf(TEXT("%.1f M/S"), GetCurrentValue());
    default:
        return FString();
    }
}
