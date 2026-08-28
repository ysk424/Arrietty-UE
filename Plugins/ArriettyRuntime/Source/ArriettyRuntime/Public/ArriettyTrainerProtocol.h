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
double AltitudeForSpeed(double SpeedKmh);
bool RequiresRideSurface(bool bFlightEnabled);
int32 CompletedLaps(double DistanceMeters, double LapLengthMeters);
double EffectiveSteeringDegrees(double FilteredRawDegrees);
double HeadingDegreesForUnrealWorldForward(const FVector2D& WorldForward);
double YawCorrectionDegrees(const FVector2D& CurrentWorldForward, const FVector2D& TargetWorldForward);
}
