// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

namespace Arrietty
{
inline constexpr TCHAR Version[] = TEXT("0.11.0");
inline constexpr TCHAR SteeringControllerSerial[] = TEXT("LHR-9EFF8645");
inline constexpr TCHAR RideSurfaceTag[] = TEXT("SecretWorldRideSurface");
inline constexpr double EyeHeightMeters = 1.5;
inline constexpr double DefaultMoveStepMeters = 0.5;
inline constexpr double DefaultTurnStepDegrees = 5.0;
inline constexpr double DefaultLapLengthMeters = 143.0;
inline constexpr double TakeoffSpeedKmh = 20.0;
inline constexpr double WheelbaseMeters = 1.05;
inline constexpr double SteeringGain = 0.50;
inline constexpr double SteeringDeadzoneDegrees = 1.5;
inline constexpr double MaxEffectiveSteeringDegrees = 15.0;
inline constexpr double BrakeGradePercent = 3.0;
inline constexpr double SampleStaleSeconds = 1.25;
inline constexpr double HeartRateStaleSeconds = 5.0;
inline constexpr double CoastStopSpeedKmh = 5.0;
inline constexpr double DefaultWheelStopSeconds = 1.5;
inline constexpr double MinWheelStopSeconds = 0.75;
inline constexpr double MaxWheelStopSeconds = 4.0;
inline constexpr double FlightEffectiveMassKg = 35.0;
inline constexpr double FlightGlideRatio = 30.0;
inline constexpr double FlightPropellerEfficiency = 0.80;
inline constexpr double FlightBestGlideSpeedKmh = 24.0;
inline constexpr double FlightStallSpeedKmh = 18.0;
inline constexpr double FlightStallRecoverySpeedKmh = 20.5;
inline constexpr double FlightMaxBankDegrees = 25.0;
inline constexpr double FlightMaxPitchDegrees = 12.0;
inline constexpr double FlightControlStepDegrees = 1.0;
inline constexpr double FlightPitchRateDegreesPerSecond = 28.0;
inline constexpr double FlightBankRateDegreesPerSecond = 55.0;
inline constexpr double FlightControlReferenceSpeedKmh = 24.0;
inline constexpr double FlightMinControlAuthority = 0.20;
inline constexpr double FlightMaxControlAuthority = 1.75;
inline constexpr double FlightMaxElevatorVerticalSpeedMps = 1.5;
inline constexpr double FlightStallSinkSpeedMps = 2.0;
inline constexpr double FlightMaxBankTurnRateDegrees = 18.0;
inline constexpr double FlightMaxRudderTurnRateDegrees = 10.0;
inline constexpr double FlightControlDeadzone = 0.08;
inline constexpr double FlightMaxPropellerThrustNewtons = 35.0;
inline constexpr double FlightGroundRollingResistance = 0.012;
inline constexpr double FlightOverspeedWarningKmh = 60.0;
inline constexpr double FanMaximumSpeedKmh = 30.0;
inline constexpr double FanStoppedThresholdKmh = 0.5;
inline constexpr double FanResendSeconds = 2.0;
inline constexpr int32 FanLevelCount = 6;
inline constexpr int32 FanUdpPort = 4210;
inline constexpr double FlightTestPropulsionPowerWatts = 95.0;
inline constexpr double FlightTestPropulsionPowerStepWatts = 5.0;
inline constexpr double FlightMinTestPropulsionPowerWatts = 0.0;
inline constexpr double FlightMaxTestPropulsionPowerWatts = 300.0;
inline constexpr double FlightPitchRateStepDegreesPerSecond = 2.0;
inline constexpr double FlightMinPitchRateDegreesPerSecond = 4.0;
inline constexpr double FlightMaxPitchRateTuningDegreesPerSecond = 60.0;
inline constexpr double FlightVerticalSpeedStepMps = 0.1;
inline constexpr double FlightMinElevatorVerticalSpeedMps = 0.3;
inline constexpr double FlightMaxElevatorVerticalSpeedTuningMps = 3.0;
inline constexpr double FlightBankRateStepDegreesPerSecond = 5.0;
inline constexpr double FlightMinBankRateDegreesPerSecond = 5.0;
inline constexpr double FlightMaxBankRateTuningDegreesPerSecond = 120.0;
inline constexpr double FlightAirspeedMultiplier = 3.0;
inline constexpr double FlightAirspeedMultiplierStep = 0.5;
inline constexpr double FlightMinAirspeedMultiplier = 1.0;
inline constexpr double FlightMaxAirspeedMultiplier = 6.0;
inline constexpr double FlightPositiveClimbMultiplier = 10.0;
inline constexpr double FlightPositiveClimbMultiplierStep = 1.0;
inline constexpr double FlightMinPositiveClimbMultiplier = 1.0;
inline constexpr double FlightMaxPositiveClimbMultiplier = 20.0;
inline constexpr int32 VoiceBridgePort = 49000;
}

enum class EArriettyFlightTuningParameter : uint8
{
    TestPropulsionPower,
    AirspeedMultiplier,
    PositiveClimbMultiplier,
    PitchResponseRate,
    ElevatorVerticalSpeed,
    BankResponseRate,
    Count
};

struct FArriettyFlightTuningValues
{
    double TestPropulsionPowerWatts = Arrietty::FlightTestPropulsionPowerWatts;
    double AirspeedMultiplier = Arrietty::FlightAirspeedMultiplier;
    double PositiveClimbMultiplier = Arrietty::FlightPositiveClimbMultiplier;
    double PitchRateDegreesPerSecond = Arrietty::FlightPitchRateDegreesPerSecond;
    double MaxElevatorVerticalSpeedMps = Arrietty::FlightMaxElevatorVerticalSpeedMps;
    double BankRateDegreesPerSecond = Arrietty::FlightBankRateDegreesPerSecond;
};

enum class EArriettyRideStatus : uint8
{
    Idle,
    Searching,
    Connecting,
    WaitingSteering,
    Riding,
    Stopping,
    Error
};

struct FArriettyTrainerSample
{
    TOptional<double> SpeedKmh;
    TOptional<double> CadenceRpm;
    TOptional<int32> PowerWatts;
};

struct FArriettyCscSample
{
    TOptional<uint32> WheelRevolutions;
    TOptional<uint16> WheelEventTimeTicks;
    TOptional<uint16> CrankRevolutions;
    TOptional<uint16> CrankEventTimeTicks;
};

struct FArriettyControlPreset
{
    int32 Index = 1;
    const TCHAR* Label = TEXT("");
    double RollingResistance = 0.004;
};

struct FArriettyFlightState
{
    double AirspeedMetersPerSecond = 0.0;
    double AltitudeMeters = 0.0;
    double VerticalSpeedMetersPerSecond = 0.0;
    double BankDegrees = 0.0;
    double PitchDegrees = 0.0;
    double FlightPathAngleDegrees = 0.0;
    double AngleOfAttackDegrees = 0.0;
    double ControlAuthority = Arrietty::FlightMinControlAuthority;
    double HeadingRateDegreesPerSecond = 0.0;
    bool bAirborne = false;
    bool bStalled = false;
};

struct FArriettyFlightStepResult
{
    bool bTookOff = false;
    bool bLanded = false;
    bool bStallStarted = false;
    bool bStallRecovered = false;
    bool bLandingBlocked = false;
};

struct FArriettyRideSnapshot
{
    EArriettyRideStatus Status = EArriettyRideStatus::Idle;
    FString Message = TEXT("Press Numpad 0 when the T2 is awake");
    FString ControlStatus = TEXT("IDLE");
    FString ControlMessage = TEXT("T2 flat-road control is idle");
    double SpeedKmh = 0.0;
    double FtmsSpeedKmh = 0.0;
    double CadenceRpm = 0.0;
    int32 PowerWatts = 0;
    double PropulsionPowerWatts = 0.0;
    TOptional<uint16> HeartRateBpm;
    FString HeartRateStatus = TEXT("NOT CONNECTED");
    FString ControllerStatus = TEXT("SEARCHING: USB controller");
    bool bControllerConnected = false;
    FVector2D ControllerJoystick1 = FVector2D::ZeroVector;
    FVector2D ControllerJoystick2 = FVector2D::ZeroVector;
    uint8 ControllerButtonMask = 0;
    double CommandedRollRightDegrees = 0.0;
    double CommandedPitchDegrees = 0.0;
    bool bFlightTuningActive = false;
    FString FlightTuningStatus = TEXT("TUNE OFF - PRESS J1 SW");
    double FlightTestPropulsionPowerWatts = Arrietty::FlightTestPropulsionPowerWatts;
    double FlightAirspeedMultiplier = Arrietty::FlightAirspeedMultiplier;
    double FlightPositiveClimbMultiplier = Arrietty::FlightPositiveClimbMultiplier;
    double FlightPitchRateDegreesPerSecond = Arrietty::FlightPitchRateDegreesPerSecond;
    double FlightMaxElevatorVerticalSpeedMps = Arrietty::FlightMaxElevatorVerticalSpeedMps;
    double FlightBankRateDegreesPerSecond = Arrietty::FlightBankRateDegreesPerSecond;
    bool bPushToTalkHeld = false;
    FString VoiceStatus = TEXT("PTT READY");
    bool bBrakeButtonHeld = false;
    double AppliedGradePercent = 0.0;
    double DistanceMeters = 0.0;
    double AltitudeMeters = 0.0;
    double VerticalSpeedMetersPerSecond = 0.0;
    double BankDegrees = 0.0;
    double PitchDegrees = 0.0;
    double FlightPathAngleDegrees = 0.0;
    double AngleOfAttackDegrees = 0.0;
    double FlightControlAuthority = Arrietty::FlightMinControlAuthority;
    bool bAircraftAirborne = false;
    bool bAircraftStalled = false;
    bool bAircraftOverspeed = false;
    int32 LapsCompleted = 0;
    bool bFlightEnabled = false;
    bool bSteeringTracking = false;
    FString SteeringSource = TEXT("NONE");
    double RawSteeringDegrees = 0.0;
    double EffectiveSteeringDegrees = 0.0;
    int32 SelectedPreset = 5;
    TOptional<int32> AppliedPreset;
    FVector2D PositionMeters = FVector2D::ZeroVector;
    double HeadingDegrees = 0.0;
    bool bGeospatialNavigation = false;
    double LongitudeDegrees = 0.0;
    double LatitudeDegrees = 0.0;
    double EllipsoidHeightMeters = 0.0;
    double AverageFps = 60.0;
};

inline FString LexToString(EArriettyRideStatus Status)
{
    switch (Status)
    {
    case EArriettyRideStatus::Idle: return TEXT("IDLE");
    case EArriettyRideStatus::Searching: return TEXT("SEARCHING");
    case EArriettyRideStatus::Connecting: return TEXT("CONNECTING");
    case EArriettyRideStatus::WaitingSteering: return TEXT("WAITING_STEERING");
    case EArriettyRideStatus::Riding: return TEXT("RIDING");
    case EArriettyRideStatus::Stopping: return TEXT("STOPPING");
    case EArriettyRideStatus::Error: return TEXT("ERROR");
    default: return TEXT("UNKNOWN");
    }
}
