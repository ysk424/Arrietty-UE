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
TArray<uint8> BuildFlatRoadControlCommand(int32 PresetIndex);
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
int32 CompletedLaps(double DistanceMeters, double LapLengthMeters);
double EffectiveSteeringDegrees(double FilteredRawDegrees);
}
