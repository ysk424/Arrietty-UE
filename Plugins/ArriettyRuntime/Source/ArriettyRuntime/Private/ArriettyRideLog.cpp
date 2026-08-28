// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyRideLog.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"

FArriettyRideLog::~FArriettyRideLog()
{
    Stop(TEXT("STOP"));
}

bool FArriettyRideLog::Start()
{
    Stop(nullptr);
    Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("arrietty_ride.csv"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    File.Reset(IFileManager::Get().CreateFileWriter(*Path));
    if (!File)
    {
        return false;
    }
    StartedAtSeconds = FPlatformTime::Seconds();
    WriteUtf8(TEXT("timestamp,elapsed_s,event,speed_kmh,ftms_speed_kmh,cadence_rpm,power_w,heart_rate_bpm,distance_m,laps_completed,flight_mode,altitude_m,target_altitude_m,xr_base_z_m,xr_navigation_z_m,xr_viewer_z_m,x_m,y_m,heading_degrees,raw_steering_degrees,effective_steering_degrees,csc_wheel_revolutions,csc_wheel_event_time_ticks,csc_wheel_stopped,low_speed_coast_stopped,t2_control_status,t2_control_preset,brake_button_held,trainer_grade_percent,steering_source\r\n"));
    FArriettyRideSnapshot Empty;
    Record(TEXT("START"), Empty, 0.0, {}, {}, false, false);
    return true;
}

void FArriettyRideLog::Record(
    const TCHAR* Event,
    const FArriettyRideSnapshot& Snapshot,
    double FtmsSpeedKmh,
    TOptional<uint32> WheelRevolutions,
    TOptional<uint16> WheelEventTicks,
    bool bWheelStopped,
    bool bLowSpeedCoastStopped)
{
    if (!File)
    {
        return;
    }
    const FString Timestamp = FDateTime::Now().ToIso8601();
    const FString WheelRevs = WheelRevolutions.IsSet() ? FString::Printf(TEXT("%u"), WheelRevolutions.GetValue()) : FString();
    const FString WheelTicks = WheelEventTicks.IsSet() ? FString::Printf(TEXT("%u"), WheelEventTicks.GetValue()) : FString();
    const FString Preset = Snapshot.AppliedPreset.IsSet() ? FString::Printf(TEXT("%d"), Snapshot.AppliedPreset.GetValue()) : FString();
    const FString HeartRate = Snapshot.HeartRateBpm.IsSet()
        ? FString::Printf(TEXT("%u"), Snapshot.HeartRateBpm.GetValue())
        : FString();
    const double EyeZ = Snapshot.AltitudeMeters + Arrietty::EyeHeightMeters;
    const double TargetAltitude = Snapshot.bFlightEnabled
        ? FMath::Max(0.0, Snapshot.SpeedKmh - Arrietty::TakeoffSpeedKmh)
        : 0.0;
    WriteUtf8(FString::Printf(
        TEXT("%s,%.3f,%s,%.2f,%.2f,%.1f,%d,%s,%.3f,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%s,%s,%d,%d,%s,%s,%d,%.1f,%s\r\n"),
        *Timestamp,
        FPlatformTime::Seconds() - StartedAtSeconds,
        Event,
        Snapshot.SpeedKmh,
        FtmsSpeedKmh,
        Snapshot.CadenceRpm,
        Snapshot.PowerWatts,
        *HeartRate,
        Snapshot.DistanceMeters,
        Snapshot.LapsCompleted,
        Snapshot.bFlightEnabled ? 1 : 0,
        Snapshot.AltitudeMeters,
        TargetAltitude,
        Arrietty::EyeHeightMeters,
        Snapshot.AltitudeMeters,
        EyeZ,
        Snapshot.PositionMeters.X,
        Snapshot.PositionMeters.Y,
        Snapshot.HeadingDegrees,
        Snapshot.RawSteeringDegrees,
        Snapshot.EffectiveSteeringDegrees,
        *WheelRevs,
        *WheelTicks,
        bWheelStopped ? 1 : 0,
        bLowSpeedCoastStopped ? 1 : 0,
        *Snapshot.ControlStatus,
        *Preset,
        Snapshot.bBrakeButtonHeld ? 1 : 0,
        Snapshot.AppliedGradePercent,
        *Snapshot.SteeringSource));
    File->Flush();
}

void FArriettyRideLog::Stop(const TCHAR* Event, const FArriettyRideSnapshot* Snapshot)
{
    if (!File)
    {
        return;
    }
    if (Event != nullptr)
    {
        FArriettyRideSnapshot Empty;
        Record(Event, Snapshot ? *Snapshot : Empty, 0.0, {}, {}, false, false);
    }
    File->Close();
    File.Reset();
    StartedAtSeconds = 0.0;
}

void FArriettyRideLog::WriteUtf8(const FString& Text)
{
    if (!File)
    {
        return;
    }
    FTCHARToUTF8 Converted(*Text);
    File->Serialize(const_cast<ANSICHAR*>(Converted.Get()), Converted.Length());
}
