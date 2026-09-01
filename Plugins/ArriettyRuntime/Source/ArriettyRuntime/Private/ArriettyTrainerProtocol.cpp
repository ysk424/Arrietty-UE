// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyTrainerProtocol.h"

namespace
{
constexpr uint8 FtmsSetIndoorBikeSimulation = 0x11;
constexpr uint8 FtmsResponseCode = 0x80;
constexpr double StandardGravityMetersPerSecondSquared = 9.80665;

double AircraftWeightNewtons()
{
    return Arrietty::FlightEffectiveMassKg * StandardGravityMetersPerSecondSquared;
}

double DynamicPressure(double AirspeedMetersPerSecond)
{
    return 0.5 * Arrietty::FlightAirDensityKgPerCubicMeter *
        FMath::Square(FMath::Max(0.0, AirspeedMetersPerSecond));
}

double MaximumLiftCoefficient()
{
    const double StallSpeed = Arrietty::FlightStallSpeedKmh / 3.6;
    return AircraftWeightNewtons() /
        (DynamicPressure(StallSpeed) * Arrietty::FlightWingAreaSquareMeters);
}

double TrimLiftCoefficient()
{
    const double BestGlideSpeed = Arrietty::FlightBestGlideSpeedKmh / 3.6;
    return AircraftWeightNewtons() /
        (DynamicPressure(BestGlideSpeed) * Arrietty::FlightWingAreaSquareMeters);
}

double WingIncidenceDegrees()
{
    return TrimLiftCoefficient() / Arrietty::FlightLiftCurveSlopePerDegree;
}

double ParasiteDragCoefficient()
{
    return TrimLiftCoefficient() / (2.0 * Arrietty::FlightGlideRatio);
}

double InducedDragFactor()
{
    return ParasiteDragCoefficient() / FMath::Square(TrimLiftCoefficient());
}

double LiftCoefficientForAngleOfAttack(double AngleOfAttackDegrees, bool bStalled)
{
    const double AttachedCoefficient = FMath::Clamp(
        AngleOfAttackDegrees * Arrietty::FlightLiftCurveSlopePerDegree,
        -MaximumLiftCoefficient(),
        MaximumLiftCoefficient());
    return bStalled
        ? AttachedCoefficient * Arrietty::FlightPostStallLiftFactor
        : AttachedCoefficient;
}

double DragCoefficientForLift(double LiftCoefficient, bool bStalled)
{
    return ParasiteDragCoefficient() +
        InducedDragFactor() * FMath::Square(LiftCoefficient) +
        (bStalled ? Arrietty::FlightPostStallDragCoefficient : 0.0);
}

void CalculateAerodynamicForces(
    double AirspeedMetersPerSecond,
    double PitchDegrees,
    double FlightPathAngleDegrees,
    bool bStalled,
    double& OutLiftNewtons,
    double& OutDragNewtons,
    double& OutAngleOfAttackDegrees)
{
    OutAngleOfAttackDegrees = FMath::Clamp(
        WingIncidenceDegrees() + PitchDegrees - FlightPathAngleDegrees,
        -45.0,
        45.0);
    const double LiftCoefficient = LiftCoefficientForAngleOfAttack(
        OutAngleOfAttackDegrees, bStalled);
    const double PressureArea =
        DynamicPressure(AirspeedMetersPerSecond) * Arrietty::FlightWingAreaSquareMeters;
    OutLiftNewtons = PressureArea * LiftCoefficient;
    OutDragNewtons = PressureArea * DragCoefficientForLift(LiftCoefficient, bStalled);
}

void UpdateFlightAttitudeMetrics(FArriettyFlightState& State)
{
    if (!State.bAirborne)
    {
        State.VerticalSpeedMetersPerSecond = 0.0;
        State.FlightPathAngleDegrees = 0.0;
        State.AngleOfAttackDegrees = 0.0;
        return;
    }

    const double FlightPathRadians = FMath::DegreesToRadians(State.FlightPathAngleDegrees);
    State.VerticalSpeedMetersPerSecond =
        State.AirspeedMetersPerSecond * FMath::Sin(FlightPathRadians);
    State.AngleOfAttackDegrees = FMath::Clamp(
        WingIncidenceDegrees() + State.PitchDegrees - State.FlightPathAngleDegrees,
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
    const double PressureArea = DynamicPressure(AirspeedMetersPerSecond) *
        Arrietty::FlightWingAreaSquareMeters;
    if (PressureArea <= UE_SMALL_NUMBER)
    {
        return 0.0;
    }
    const double RequiredLiftCoefficient = FMath::Clamp(
        AircraftWeightNewtons() / PressureArea,
        0.0,
        MaximumLiftCoefficient());
    return PressureArea * DragCoefficientForLift(RequiredLiftCoefficient, false);
}

double ArriettyTrainerProtocol::HumanPoweredFlightLiftNewtons(
    double AirspeedMetersPerSecond,
    double PitchDegrees,
    double FlightPathAngleDegrees,
    bool bStalled)
{
    double Lift = 0.0;
    double Drag = 0.0;
    double AngleOfAttack = 0.0;
    CalculateAerodynamicForces(
        AirspeedMetersPerSecond,
        PitchDegrees,
        FlightPathAngleDegrees,
        bStalled,
        Lift,
        Drag,
        AngleOfAttack);
    return Lift;
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
    double TargetPitchDegrees,
    double TargetBankDegrees,
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

    const double Power = FMath::Max(0.0, RiderPowerWatts);
    double Airspeed = FMath::Max(0.0, State.AirspeedMetersPerSecond);
    const double EffectivePower = Power * Arrietty::FlightPropellerEfficiency;
    State.ControlAuthority = HumanPoweredFlightControlAuthority(Airspeed, State.bStalled);
    const double CommandedPitch = FMath::Clamp(
        TargetPitchDegrees,
        -Arrietty::FlightMaxPitchDegrees,
        Arrietty::FlightMaxPitchDegrees);
    const double CommandedBank = FMath::Clamp(
        TargetBankDegrees,
        -Arrietty::FlightMaxBankDegrees,
        Arrietty::FlightMaxBankDegrees);
    const double TargetPitch = State.bStalled
        ? FMath::Min(CommandedPitch, -10.0)
        : CommandedPitch;
    const double TargetBank = State.bAirborne ? CommandedBank : 0.0;
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
    double Lift = 0.0;
    double Drag = 0.0;
    double AngleOfAttack = 0.0;
    CalculateAerodynamicForces(
        Airspeed,
        State.PitchDegrees,
        State.FlightPathAngleDegrees,
        State.bStalled,
        Lift,
        Drag,
        AngleOfAttack);

    if (!State.bAirborne)
    {
        State.AltitudeMeters = 0.0;
        State.VerticalSpeedMetersPerSecond = 0.0;
        State.FlightPathAngleDegrees = 0.0;
        State.bStalled = false;
        const double Thrust = Power <= 0.0
            ? 0.0
            : FMath::Min(
                Arrietty::FlightMaxPropellerThrustNewtons,
                EffectivePower / FMath::Max(2.0, Airspeed));
        const double WheelLoad = FMath::Max(0.0, AircraftWeightNewtons() - Lift);
        const double GroundDrag = Drag +
            WheelLoad * Arrietty::FlightGroundRollingResistance;
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
        if (Airspeed * 3.6 >= Arrietty::TakeoffSpeedKmh &&
            CommandedPitch > 0.0 &&
            Lift >= AircraftWeightNewtons())
        {
            State.bAirborne = true;
            Result.bTookOff = true;
        }
        State.AirspeedMetersPerSecond = Airspeed;
        State.ControlAuthority = HumanPoweredFlightControlAuthority(Airspeed, State.bStalled);
        UpdateFlightAttitudeMetrics(State);
        return Result;
    }

    const bool bWasStalled = State.bStalled;
    const double StallAngleOfAttack =
        MaximumLiftCoefficient() / Arrietty::FlightLiftCurveSlopePerDegree;
    if (!State.bStalled &&
        (Airspeed * 3.6 < Arrietty::FlightStallSpeedKmh ||
         AngleOfAttack >= StallAngleOfAttack))
    {
        State.bStalled = true;
    }
    else if (State.bStalled &&
        Airspeed * 3.6 >= Arrietty::FlightStallRecoverySpeedKmh &&
        AngleOfAttack <= StallAngleOfAttack * 0.80 &&
        CommandedPitch <= 0.0)
    {
        State.bStalled = false;
    }
    Result.bStallStarted = !bWasStalled && State.bStalled;
    Result.bStallRecovered = bWasStalled && !State.bStalled;

    // Recalculate after a stall transition because separated flow changes
    // both lift and drag immediately.
    CalculateAerodynamicForces(
        Airspeed,
        State.PitchDegrees,
        State.FlightPathAngleDegrees,
        State.bStalled,
        Lift,
        Drag,
        AngleOfAttack);

    const double AerodynamicPowerRequired = Drag * Airspeed;
    const double RawSurplusPower = EffectivePower - AerodynamicPowerRequired;
    const double SimulatedEffectivePower = EffectivePower +
        FMath::Max(0.0, RawSurplusPower) *
            (FMath::Max(1.0, Tuning.PositiveClimbMultiplier) - 1.0);
    const double FlightPathRadians =
        FMath::DegreesToRadians(State.FlightPathAngleDegrees);
    const double PropulsiveForce = SimulatedEffectivePower / FMath::Max(3.0, Airspeed);
    const double AlongPathAcceleration = FMath::Clamp(
        (PropulsiveForce - Drag -
            AircraftWeightNewtons() * FMath::Sin(FlightPathRadians)) /
            Arrietty::FlightEffectiveMassKg,
        -3.0,
        3.0);
    Airspeed = FMath::Max(0.0, Airspeed + AlongPathAcceleration * Delta);

    const double BankRadians = FMath::DegreesToRadians(State.BankDegrees);
    const double NormalForce = Lift * FMath::Cos(BankRadians) -
        AircraftWeightNewtons() * FMath::Cos(FlightPathRadians);
    const double FlightPathRateRadiansPerSecond = FMath::Clamp(
        NormalForce /
            (Arrietty::FlightEffectiveMassKg * FMath::Max(3.0, Airspeed)),
        FMath::DegreesToRadians(-45.0),
        FMath::DegreesToRadians(45.0));
    State.FlightPathAngleDegrees = FMath::Clamp(
        State.FlightPathAngleDegrees +
            FMath::RadiansToDegrees(FlightPathRateRadiansPerSecond) * Delta,
        -45.0,
        45.0);
    State.AirspeedMetersPerSecond = Airspeed;
    State.ControlAuthority = HumanPoweredFlightControlAuthority(Airspeed, State.bStalled);

    const double UpdatedFlightPathRadians =
        FMath::DegreesToRadians(State.FlightPathAngleDegrees);
    const double BankTurnRate = FMath::RadiansToDegrees(
        Lift * FMath::Sin(BankRadians) /
        (Arrietty::FlightEffectiveMassKg * FMath::Max(3.0, Airspeed) *
            FMath::Max(0.2, FMath::Cos(UpdatedFlightPathRadians))));
    State.HeadingRateDegreesPerSecond = FMath::Clamp(
        BankTurnRate +
            RudderInput * Arrietty::FlightMaxRudderTurnRateDegrees * State.ControlAuthority,
        -60.0,
        60.0);
    UpdateFlightAttitudeMetrics(State);
    State.AltitudeMeters += State.VerticalSpeedMetersPerSecond * Delta;

    if (State.AltitudeMeters <= 0.0 && State.VerticalSpeedMetersPerSecond <= 0.0)
    {
        if (bCanLand)
        {
            State.AltitudeMeters = 0.0;
            State.VerticalSpeedMetersPerSecond = 0.0;
            State.FlightPathAngleDegrees = 0.0;
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
            State.FlightPathAngleDegrees = 0.0;
            Result.bLandingBlocked = true;
        }
    }
    UpdateFlightAttitudeMetrics(State);
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
