// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyTrainerProtocol.h"

namespace
{
constexpr uint8 FtmsSetIndoorBikeSimulation = 0x11;
constexpr uint8 FtmsResponseCode = 0x80;
constexpr double StandardGravityMetersPerSecondSquared = 9.80665;

double NormalizedFlightControl(double Input)
{
    const double Clamped = FMath::Clamp(Input, -1.0, 1.0);
    const double Magnitude = FMath::Abs(Clamped);
    if (Magnitude <= Arrietty::FlightControlDeadzone)
    {
        return 0.0;
    }
    return FMath::Sign(Clamped) *
        (Magnitude - Arrietty::FlightControlDeadzone) /
        (1.0 - Arrietty::FlightControlDeadzone);
}

void UpdateFlightAttitudeMetrics(
    FArriettyFlightState& State,
    double AirspeedMultiplier)
{
    if (!State.bAirborne)
    {
        State.FlightPathAngleDegrees = 0.0;
        State.AngleOfAttackDegrees = 0.0;
        return;
    }

    const double HorizontalSpeed = FMath::Sqrt(FMath::Max(
        0.0,
        FMath::Square(State.AirspeedMetersPerSecond) -
            FMath::Square(State.VerticalSpeedMetersPerSecond))) *
        FMath::Max(0.1, AirspeedMultiplier);
    State.FlightPathAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(
        State.VerticalSpeedMetersPerSecond,
        FMath::Max(0.01, HorizontalSpeed)));
    State.AngleOfAttackDegrees = FMath::Clamp(
        FMath::UnwindDegrees(State.PitchDegrees - State.FlightPathAngleDegrees),
        -45.0,
        45.0);
}

bool TakeUnsigned(TArrayView<const uint8> Data, int32& Offset, int32 Size, uint32& OutValue)
{
    if (Offset < 0 || Size <= 0 || Offset + Size > Data.Num())
    {
        Offset = Data.Num();
        return false;
    }
    OutValue = 0;
    for (int32 ByteIndex = 0; ByteIndex < Size; ++ByteIndex)
    {
        OutValue |= static_cast<uint32>(Data[Offset + ByteIndex]) << (8 * ByteIndex);
    }
    Offset += Size;
    return true;
}
}

const TArray<FArriettyControlPreset>& ArriettyTrainerProtocol::Presets()
{
    static const TArray<FArriettyControlPreset> Values = {
        {1, TEXT("Race"), 0.0040},
        {2, TEXT("Road"), 0.0080},
        {3, TEXT("Firm"), 0.0120},
        {4, TEXT("Strong"), 0.0160},
        {5, TEXT("Road Default"), 0.0200},
        {6, TEXT("Bicycle"), 0.0240},
        {7, TEXT("FTMS Limit"), 0.0255},
    };
    return Values;
}

const FArriettyControlPreset* ArriettyTrainerProtocol::FindPreset(int32 PresetIndex)
{
    return Presets().FindByPredicate(
        [PresetIndex](const FArriettyControlPreset& Preset) { return Preset.Index == PresetIndex; });
}

bool ArriettyTrainerProtocol::ParseIndoorBikeData(
    TArrayView<const uint8> Data,
    FArriettyTrainerSample& OutSample)
{
    if (Data.Num() < 2)
    {
        return false;
    }

    const uint16 Flags = static_cast<uint16>(Data[0]) | (static_cast<uint16>(Data[1]) << 8);
    int32 Offset = 2;
    uint32 Value = 0;

    if ((Flags & 0x0001) == 0)
    {
        if (!TakeUnsigned(Data, Offset, 2, Value)) return false;
        OutSample.SpeedKmh = Value * 0.01;
    }
    if ((Flags & 0x0002) != 0 && !TakeUnsigned(Data, Offset, 2, Value)) return false;
    if ((Flags & 0x0004) != 0)
    {
        if (!TakeUnsigned(Data, Offset, 2, Value)) return false;
        OutSample.CadenceRpm = Value * 0.5;
    }
    if ((Flags & 0x0008) != 0 && !TakeUnsigned(Data, Offset, 2, Value)) return false;
    if ((Flags & 0x0010) != 0 && !TakeUnsigned(Data, Offset, 3, Value)) return false;
    if ((Flags & 0x0020) != 0 && !TakeUnsigned(Data, Offset, 2, Value)) return false;
    if ((Flags & 0x0040) != 0)
    {
        if (!TakeUnsigned(Data, Offset, 2, Value)) return false;
        OutSample.PowerWatts = static_cast<int16>(static_cast<uint16>(Value));
    }
    return true;
}

bool ArriettyTrainerProtocol::ParseCscMeasurement(
    TArrayView<const uint8> Data,
    FArriettyCscSample& OutSample)
{
    if (Data.IsEmpty())
    {
        return false;
    }
    const uint8 Flags = Data[0];
    int32 Offset = 1;
    uint32 Value = 0;
    if ((Flags & 0x01) != 0)
    {
        if (!TakeUnsigned(Data, Offset, 4, Value)) return false;
        OutSample.WheelRevolutions = Value;
        if (!TakeUnsigned(Data, Offset, 2, Value)) return false;
        OutSample.WheelEventTimeTicks = static_cast<uint16>(Value);
    }
    if ((Flags & 0x02) != 0)
    {
        if (!TakeUnsigned(Data, Offset, 2, Value)) return false;
        OutSample.CrankRevolutions = static_cast<uint16>(Value);
        if (!TakeUnsigned(Data, Offset, 2, Value)) return false;
        OutSample.CrankEventTimeTicks = static_cast<uint16>(Value);
    }
    return true;
}

TOptional<uint16> ArriettyTrainerProtocol::ParseHeartRateMeasurement(
    TArrayView<const uint8> Data)
{
    if (Data.Num() < 2)
    {
        return {};
    }
    const bool bSixteenBitValue = (Data[0] & 0x01) != 0;
    if (!bSixteenBitValue)
    {
        return static_cast<uint16>(Data[1]);
    }
    if (Data.Num() < 3)
    {
        return {};
    }
    return static_cast<uint16>(Data[1]) |
        (static_cast<uint16>(Data[2]) << 8);
}

TArray<uint8> ArriettyTrainerProtocol::BuildFlatRoadControlCommand(int32 PresetIndex)
{
    return BuildSimulationControlCommand(PresetIndex, 0.0);
}

TArray<uint8> ArriettyTrainerProtocol::BuildSimulationControlCommand(
    int32 PresetIndex,
    double GradePercent)
{
    const FArriettyControlPreset* Preset = FindPreset(PresetIndex);
    if (Preset == nullptr)
    {
        return {};
    }
    const int16 WindSpeed = 0;
    const int16 Grade = static_cast<int16>(FMath::Clamp(
        FMath::RoundToInt(GradePercent * 100.0),
        static_cast<int32>(MIN_int16),
        static_cast<int32>(MAX_int16)));
    const uint16 GradeBits = static_cast<uint16>(Grade);
    const uint8 RollingResistance = static_cast<uint8>(FMath::RoundToInt(Preset->RollingResistance / 0.0001));
    const uint8 WindResistance = 51;
    return {
        FtmsSetIndoorBikeSimulation,
        static_cast<uint8>(WindSpeed & 0xff),
        static_cast<uint8>((WindSpeed >> 8) & 0xff),
        static_cast<uint8>(GradeBits & 0xff),
        static_cast<uint8>((GradeBits >> 8) & 0xff),
        RollingResistance,
        WindResistance,
    };
}

TOptional<uint8> ArriettyTrainerProtocol::ParseControlResponse(
    TArrayView<const uint8> Data,
    uint8 RequestedOpcode)
{
    if (Data.Num() < 3 || Data[0] != FtmsResponseCode || Data[1] != RequestedOpcode)
    {
        return {};
    }
    return Data[2];
}

FString ArriettyTrainerProtocol::ControlResultName(uint8 ResultCode)
{
    switch (ResultCode)
    {
    case 0x01: return TEXT("success");
    case 0x02: return TEXT("not supported");
    case 0x03: return TEXT("invalid parameter");
    case 0x04: return TEXT("operation failed");
    case 0x05: return TEXT("control not permitted");
    default: return FString::Printf(TEXT("unknown result 0x%02x"), ResultCode);
    }
}

double ArriettyTrainerProtocol::WheelStopTimeoutSeconds(double WheelPeriodSeconds)
{
    if (WheelPeriodSeconds <= 0.0)
    {
        return Arrietty::DefaultWheelStopSeconds;
    }
    return FMath::Clamp(
        WheelPeriodSeconds * 1.5 + 0.25,
        Arrietty::MinWheelStopSeconds,
        Arrietty::MaxWheelStopSeconds);
}

double ArriettyTrainerProtocol::EffectiveSpeedKmh(
    double NowSeconds,
    double LastFtmsSampleSeconds,
    double FtmsSpeedKmh,
    double CadenceRpm,
    bool bWheelSignalReceived,
    double LastWheelMotionSeconds,
    double WheelPeriodSeconds)
{
    if (NowSeconds - LastFtmsSampleSeconds > Arrietty::SampleStaleSeconds)
    {
        return 0.0;
    }
    if (bWheelSignalReceived &&
        NowSeconds - LastWheelMotionSeconds > WheelStopTimeoutSeconds(WheelPeriodSeconds))
    {
        return 0.0;
    }
    if (FtmsSpeedKmh > 0.0 && FtmsSpeedKmh <= Arrietty::CoastStopSpeedKmh && CadenceRpm <= 0.0)
    {
        return 0.0;
    }
    return FMath::Max(0.0, FtmsSpeedKmh);
}

void ArriettyTrainerProtocol::InitializeHumanPoweredFlight(
    FArriettyFlightState& State,
    double InitialAirspeedKmh)
{
    State = FArriettyFlightState();
    State.AirspeedMetersPerSecond = FMath::Max(0.0, InitialAirspeedKmh / 3.6);
}

double ArriettyTrainerProtocol::HumanPoweredFlightDragNewtons(
    double AirspeedMetersPerSecond)
{
    const double BestGlideSpeed = Arrietty::FlightBestGlideSpeedKmh / 3.6;
    const double SpeedRatio = FMath::Clamp(
        AirspeedMetersPerSecond / BestGlideSpeed,
        0.50,
        2.0);
    const double MinimumDrag =
        Arrietty::FlightEffectiveMassKg * StandardGravityMetersPerSecondSquared /
        Arrietty::FlightGlideRatio;
    return MinimumDrag * 0.5 *
        (FMath::Square(SpeedRatio) + 1.0 / FMath::Square(SpeedRatio));
}

double ArriettyTrainerProtocol::HumanPoweredLevelFlightPowerWatts(double AirspeedKmh)
{
    const double Airspeed = FMath::Max(0.0, AirspeedKmh / 3.6);
    return HumanPoweredFlightDragNewtons(Airspeed) * Airspeed /
        Arrietty::FlightPropellerEfficiency;
}

double ArriettyTrainerProtocol::HumanPoweredFlightPowerClimbRateMetersPerSecond(
    double PropulsionPowerWatts,
    double EnergyAirspeedKmh,
    double PositiveClimbMultiplier)
{
    const double Airspeed = FMath::Max(0.0, EnergyAirspeedKmh / 3.6);
    const double EffectivePower =
        FMath::Max(0.0, PropulsionPowerWatts) * Arrietty::FlightPropellerEfficiency;
    const double PowerBalanceWatts =
        EffectivePower - HumanPoweredFlightDragNewtons(Airspeed) * Airspeed;
    const double VirtualPowerBalanceWatts = PowerBalanceWatts > 0.0
        ? PowerBalanceWatts * FMath::Max(1.0, PositiveClimbMultiplier)
        : PowerBalanceWatts;
    return VirtualPowerBalanceWatts /
        (Arrietty::FlightEffectiveMassKg * StandardGravityMetersPerSecondSquared);
}

double ArriettyTrainerProtocol::HumanPoweredFlightControlAuthority(
    double AirspeedMetersPerSecond,
    bool bStalled)
{
    const double ReferenceSpeed = Arrietty::FlightControlReferenceSpeedKmh / 3.6;
    const double DynamicPressureRatio = FMath::Square(
        FMath::Max(0.0, AirspeedMetersPerSecond) / ReferenceSpeed);
    const double NormalAuthority = FMath::Clamp(
        DynamicPressureRatio,
        Arrietty::FlightMinControlAuthority,
        Arrietty::FlightMaxControlAuthority);
    return NormalAuthority * (bStalled ? 0.25 : 1.0);
}

double ArriettyTrainerProtocol::HumanPoweredFlightPropulsionPowerWatts(double RiderPowerWatts)
{
    return FMath::Max(0.0, RiderPowerWatts);
}

FArriettyFlightStepResult ArriettyTrainerProtocol::StepHumanPoweredFlight(
    FArriettyFlightState& State,
    double RiderPowerWatts,
    double ElevatorInput,
    double AileronInput,
    double RudderDegrees,
    double DeltaSeconds,
    bool bCanLand,
    const FArriettyFlightTuningValues& Tuning)
{
    FArriettyFlightStepResult Result;
    const double Delta = FMath::Clamp(DeltaSeconds, 0.0, 0.25);
    if (Delta <= 0.0)
    {
        return Result;
    }

    const double Elevator = NormalizedFlightControl(ElevatorInput);
    const double Aileron = NormalizedFlightControl(AileronInput);
    const double Power = FMath::Max(0.0, RiderPowerWatts);
    double Airspeed = FMath::Max(0.0, State.AirspeedMetersPerSecond);
    const double EffectivePower = Power * Arrietty::FlightPropellerEfficiency;
    State.ControlAuthority = HumanPoweredFlightControlAuthority(Airspeed, State.bStalled);
    const double TargetPitch = State.bStalled
        ? -10.0
        : Elevator * Arrietty::FlightMaxPitchDegrees;
    const double TargetBank = State.bAirborne
        ? Aileron * Arrietty::FlightMaxBankDegrees
        : 0.0;
    const double PitchAuthority = State.bStalled
        ? FMath::Max(0.75, State.ControlAuthority)
        : State.ControlAuthority;
    State.PitchDegrees = FMath::FInterpConstantTo(
        State.PitchDegrees,
        TargetPitch,
        Delta,
        FMath::Max(0.1, Tuning.PitchRateDegreesPerSecond) * PitchAuthority);
    State.BankDegrees = FMath::FInterpConstantTo(
        State.BankDegrees,
        TargetBank,
        Delta,
        FMath::Max(0.1, Tuning.BankRateDegreesPerSecond) * State.ControlAuthority);

    const double RudderInput = FMath::Clamp(
        RudderDegrees / Arrietty::MaxEffectiveSteeringDegrees,
        -1.0,
        1.0);
    if (!State.bAirborne)
    {
        State.AltitudeMeters = 0.0;
        State.VerticalSpeedMetersPerSecond = 0.0;
        State.bStalled = false;
        const double Thrust = Power <= 0.0
            ? 0.0
            : FMath::Min(
                Arrietty::FlightMaxPropellerThrustNewtons,
                EffectivePower / FMath::Max(2.0, Airspeed));
        const double BestGlideSpeed = Arrietty::FlightBestGlideSpeedKmh / 3.6;
        const double SpeedRatio = Airspeed / BestGlideSpeed;
        const double MinimumDrag =
            Arrietty::FlightEffectiveMassKg * StandardGravityMetersPerSecondSquared /
            Arrietty::FlightGlideRatio;
        const double GroundDrag =
            Arrietty::FlightEffectiveMassKg * StandardGravityMetersPerSecondSquared *
                Arrietty::FlightGroundRollingResistance +
            MinimumDrag * 0.5 * FMath::Square(SpeedRatio);
        double Acceleration = (Thrust - GroundDrag) / Arrietty::FlightEffectiveMassKg;
        if (Airspeed <= 0.05 && Thrust <= GroundDrag)
        {
            Acceleration = 0.0;
        }
        Airspeed = FMath::Max(0.0, Airspeed + Acceleration * Delta);
        const double TakeoffFraction = FMath::Clamp(
            Airspeed / (Arrietty::TakeoffSpeedKmh / 3.6), 0.0, 1.0);
        State.HeadingRateDegreesPerSecond =
            RudderInput * Arrietty::FlightMaxRudderTurnRateDegrees * TakeoffFraction;
        if (Airspeed * 3.6 >= Arrietty::TakeoffSpeedKmh && Elevator > 0.0)
        {
            State.bAirborne = true;
            State.VerticalSpeedMetersPerSecond = FMath::Max(
                0.25,
                Elevator * FMath::Max(0.1, Tuning.MaxElevatorVerticalSpeedMps));
            Result.bTookOff = true;
        }
        State.AirspeedMetersPerSecond = Airspeed;
        State.ControlAuthority = HumanPoweredFlightControlAuthority(Airspeed, State.bStalled);
        UpdateFlightAttitudeMetrics(State, Tuning.AirspeedMultiplier);
        return Result;
    }

    const bool bWasStalled = State.bStalled;
    if (!State.bStalled && Airspeed * 3.6 < Arrietty::FlightStallSpeedKmh)
    {
        State.bStalled = true;
    }
    else if (State.bStalled &&
        Airspeed * 3.6 >= Arrietty::FlightStallRecoverySpeedKmh &&
        Elevator <= 0.0)
    {
        State.bStalled = false;
    }
    Result.bStallStarted = !bWasStalled && State.bStalled;
    Result.bStallRecovered = bWasStalled && !State.bStalled;

    const double BaseVerticalSpeed = HumanPoweredFlightPowerClimbRateMetersPerSecond(
        Power,
        Airspeed * 3.6,
        Tuning.PositiveClimbMultiplier);
    const double VirtualPowerBalanceWatts = BaseVerticalSpeed *
        Arrietty::FlightEffectiveMassKg * StandardGravityMetersPerSecondSquared;
    double DesiredVerticalSpeed = BaseVerticalSpeed +
        Elevator * FMath::Max(0.1, Tuning.MaxElevatorVerticalSpeedMps);
    const double NormalVerticalLimit = FMath::Max(
        0.25,
        Airspeed * FMath::Sin(FMath::DegreesToRadians(Arrietty::FlightMaxPitchDegrees)));
    const double DownwardLimit = State.bStalled
        ? FMath::Max(NormalVerticalLimit, Arrietty::FlightStallSinkSpeedMps)
        : NormalVerticalLimit;
    if (State.bStalled)
    {
        DesiredVerticalSpeed = FMath::Min(
            DesiredVerticalSpeed,
            -Arrietty::FlightStallSinkSpeedMps);
    }
    DesiredVerticalSpeed = FMath::Clamp(
        DesiredVerticalSpeed,
        -DownwardLimit,
        NormalVerticalLimit);
    State.VerticalSpeedMetersPerSecond = FMath::FInterpTo(
        State.VerticalSpeedMetersPerSecond,
        DesiredVerticalSpeed,
        Delta,
        State.bStalled ? 5.0 : 2.5);

    const double ForceBalance = VirtualPowerBalanceWatts -
        Arrietty::FlightEffectiveMassKg * StandardGravityMetersPerSecondSquared *
            DesiredVerticalSpeed;
    const double Acceleration = FMath::Clamp(
        ForceBalance /
            (Arrietty::FlightEffectiveMassKg * FMath::Max(3.0, Airspeed)),
        -3.0,
        3.0);
    Airspeed = FMath::Max(0.0, Airspeed + Acceleration * Delta);
    State.AirspeedMetersPerSecond = Airspeed;
    State.ControlAuthority = HumanPoweredFlightControlAuthority(Airspeed, State.bStalled);

    const double BankTurnRate = FMath::Clamp(
        FMath::RadiansToDegrees(
            StandardGravityMetersPerSecondSquared *
            FMath::Tan(FMath::DegreesToRadians(State.BankDegrees)) /
            FMath::Max(4.0, Airspeed)),
        -Arrietty::FlightMaxBankTurnRateDegrees,
        Arrietty::FlightMaxBankTurnRateDegrees);
    const double BankAuthority = State.bStalled ? 0.25 : 1.0;
    State.HeadingRateDegreesPerSecond = FMath::Clamp(
        BankTurnRate * BankAuthority +
            RudderInput * Arrietty::FlightMaxRudderTurnRateDegrees * State.ControlAuthority,
        -24.0,
        24.0);
    State.AltitudeMeters += State.VerticalSpeedMetersPerSecond * Delta;

    if (State.AltitudeMeters <= 0.0 && State.VerticalSpeedMetersPerSecond <= 0.0)
    {
        if (bCanLand)
        {
            State.AltitudeMeters = 0.0;
            State.VerticalSpeedMetersPerSecond = 0.0;
            State.BankDegrees = 0.0;
            State.PitchDegrees = 0.0;
            State.HeadingRateDegreesPerSecond = 0.0;
            State.bAirborne = false;
            State.bStalled = false;
            Result.bLanded = true;
        }
        else
        {
            State.AltitudeMeters = 0.5;
            State.VerticalSpeedMetersPerSecond = 0.0;
            Result.bLandingBlocked = true;
        }
    }
    UpdateFlightAttitudeMetrics(State, Tuning.AirspeedMultiplier);
    return Result;
}

bool ArriettyTrainerProtocol::RequiresRideSurface(bool bFlightEnabled)
{
    return !bFlightEnabled;
}

int32 ArriettyTrainerProtocol::CompletedLaps(double DistanceMeters, double LapLengthMeters)
{
    return FMath::Max(0, FMath::FloorToInt(DistanceMeters / FMath::Max(1.0, LapLengthMeters)));
}

double ArriettyTrainerProtocol::EffectiveSteeringDegrees(double FilteredRawDegrees)
{
    const double Magnitude = FMath::Max(0.0, FMath::Abs(FilteredRawDegrees) - Arrietty::SteeringDeadzoneDegrees);
    const double Effective = FMath::Sign(FilteredRawDegrees) * Magnitude * Arrietty::SteeringGain;
    return FMath::Clamp(Effective, -Arrietty::MaxEffectiveSteeringDegrees, Arrietty::MaxEffectiveSteeringDegrees);
}

double ArriettyTrainerProtocol::HeadingDegreesForUnrealWorldForward(
    const FVector2D& WorldForward)
{
    if (WorldForward.IsNearlyZero())
    {
        return 0.0;
    }
    // Arrietty simulation Y becomes Unreal -Y in ArriettyToWorld, so an
    // Unreal world yaw has the opposite sign from ride HeadingDegrees.
    return FMath::UnwindDegrees(-FMath::RadiansToDegrees(
        FMath::Atan2(WorldForward.Y, WorldForward.X)));
}

double ArriettyTrainerProtocol::YawCorrectionDegrees(
    const FVector2D& CurrentWorldForward,
    const FVector2D& TargetWorldForward)
{
    const FVector2D Current = CurrentWorldForward.GetSafeNormal();
    const FVector2D Target = TargetWorldForward.GetSafeNormal();
    if (Current.IsNearlyZero() || Target.IsNearlyZero())
    {
        return 0.0;
    }
    return FMath::RadiansToDegrees(FMath::Atan2(
        Current.X * Target.Y - Current.Y * Target.X,
        FVector2D::DotProduct(Current, Target)));
}

double ArriettyTrainerProtocol::HmdOriginYawDegrees(
    const FVector2D& HmdTrackingForward)
{
    return YawCorrectionDegrees(HmdTrackingForward, FVector2D(1.0, 0.0));
}
