// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyTrainerProtocol.h"

namespace
{
constexpr uint8 FtmsSetIndoorBikeSimulation = 0x11;
constexpr uint8 FtmsResponseCode = 0x80;

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

TArray<uint8> ArriettyTrainerProtocol::BuildFlatRoadControlCommand(int32 PresetIndex)
{
    const FArriettyControlPreset* Preset = FindPreset(PresetIndex);
    if (Preset == nullptr)
    {
        return {};
    }
    const int16 WindSpeed = 0;
    const int16 Grade = 0;
    const uint8 RollingResistance = static_cast<uint8>(FMath::RoundToInt(Preset->RollingResistance / 0.0001));
    const uint8 WindResistance = 51;
    return {
        FtmsSetIndoorBikeSimulation,
        static_cast<uint8>(WindSpeed & 0xff),
        static_cast<uint8>((WindSpeed >> 8) & 0xff),
        static_cast<uint8>(Grade & 0xff),
        static_cast<uint8>((Grade >> 8) & 0xff),
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

double ArriettyTrainerProtocol::AltitudeForSpeed(double SpeedKmh)
{
    return FMath::Max(0.0, SpeedKmh - Arrietty::TakeoffSpeedKmh);
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
