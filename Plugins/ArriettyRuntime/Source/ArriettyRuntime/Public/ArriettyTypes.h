// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

namespace Arrietty
{
inline constexpr TCHAR Version[] = TEXT("0.8.0");
inline constexpr TCHAR RightControllerSerial[] = TEXT("LHR-9EFF8645");
inline constexpr TCHAR RideSurfaceTag[] = TEXT("SecretWorldRideSurface");
inline constexpr double EyeHeightMeters = 1.5;
inline constexpr double DefaultMoveStepMeters = 0.5;
inline constexpr double DefaultTurnStepDegrees = 5.0;
inline constexpr double DefaultLapLengthMeters = 143.0;
inline constexpr double TakeoffSpeedKmh = 10.0;
inline constexpr double WheelbaseMeters = 1.05;
inline constexpr double SteeringGain = 0.50;
inline constexpr double SteeringDeadzoneDegrees = 1.5;
inline constexpr double MaxEffectiveSteeringDegrees = 15.0;
inline constexpr double SampleStaleSeconds = 1.25;
inline constexpr double HeartRateStaleSeconds = 5.0;
inline constexpr double CoastStopSpeedKmh = 5.0;
inline constexpr double DefaultWheelStopSeconds = 1.5;
inline constexpr double MinWheelStopSeconds = 0.75;
inline constexpr double MaxWheelStopSeconds = 4.0;
}

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
    TOptional<uint16> HeartRateBpm;
    FString HeartRateStatus = TEXT("NOT CONNECTED");
    double DistanceMeters = 0.0;
    double AltitudeMeters = 0.0;
    int32 LapsCompleted = 0;
    bool bFlightEnabled = false;
    bool bSteeringTracking = false;
    double RawSteeringDegrees = 0.0;
    double EffectiveSteeringDegrees = 0.0;
    int32 SelectedPreset = 5;
    TOptional<int32> AppliedPreset;
    FVector2D PositionMeters = FVector2D::ZeroVector;
    double HeadingDegrees = 0.0;
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
