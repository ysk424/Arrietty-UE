// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyTypes.h"

namespace ArriettyTrainerProtocol
{
const TArray<FArriettyControlPreset>& Presets();
const FArriettyControlPreset* FindPreset(int32 PresetIndex);
bool ParseIndoorBikeData(TArrayView<const uint8> Data, FArriettyTrainerSample& OutSample);
bool ParseCscMeasurement(TArrayView<const uint8> Data, FArriettyCscSample& OutSample);
TOptional<uint16> ParseHeartRateMeasurement(TArrayView<const uint8> Data);
TArray<uint8> BuildFlatRoadControlCommand(int32 PresetIndex);
TArray<uint8> BuildSimulationControlCommand(int32 PresetIndex, double GradePercent);
TOptional<uint8> ParseControlResponse(TArrayView<const uint8> Data, uint8 RequestedOpcode);
FString ControlResultName(uint8 ResultCode);
double WheelStopTimeoutSeconds(double WheelPeriodSeconds);
double EffectiveSpeedKmh(
    double NowSeconds,
    double LastFtmsSampleSeconds,
    double FtmsSpeedKmh,
    double CadenceRpm,
    bool bWheelSignalReceived,
    double LastWheelMotionSeconds,
    double WheelPeriodSeconds);
void InitializeHumanPoweredFlight(FArriettyFlightState& State, double InitialAirspeedKmh = 0.0);
double HumanPoweredFlightDragNewtons(double AirspeedMetersPerSecond);
double HumanPoweredLevelFlightPowerWatts(double AirspeedKmh);
double HumanPoweredFlightPowerClimbRateMetersPerSecond(
    double PropulsionPowerWatts,
    double EnergyAirspeedKmh,
    double PositiveClimbMultiplier);
double HumanPoweredFlightControlAuthority(double AirspeedMetersPerSecond, bool bStalled);
double HumanPoweredFlightPropulsionPowerWatts(double RiderPowerWatts);
FArriettyFlightStepResult StepHumanPoweredFlight(
    FArriettyFlightState& State,
    double RiderPowerWatts,
    double ElevatorInput,
    double AileronInput,
    double RudderDegrees,
    double DeltaSeconds,
    bool bCanLand,
    const FArriettyFlightTuningValues& Tuning = FArriettyFlightTuningValues());
bool RequiresRideSurface(bool bFlightEnabled);
int32 CompletedLaps(double DistanceMeters, double LapLengthMeters);
double EffectiveSteeringDegrees(double FilteredRawDegrees);
double HeadingDegreesForUnrealWorldForward(const FVector2D& WorldForward);
double YawCorrectionDegrees(const FVector2D& CurrentWorldForward, const FVector2D& TargetWorldForward);
double HmdOriginYawDegrees(const FVector2D& HmdTrackingForward);
}
