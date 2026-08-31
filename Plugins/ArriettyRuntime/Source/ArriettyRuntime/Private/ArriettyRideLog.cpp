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
    WriteUtf8(TEXT("timestamp,elapsed_s,event,speed_kmh,ftms_speed_kmh,cadence_rpm,rider_power_w,propulsion_power_w,heart_rate_bpm,distance_m,laps_completed,flight_mode,airborne,stalled,overspeed,altitude_agl_m,vertical_speed_mps,flight_path_angle_degrees,angle_of_attack_degrees,control_authority,bank_degrees,pitch_degrees,commanded_roll_right_degrees,commanded_pitch_up_degrees,flight_tuning_active,flight_tuning_status,test_propulsion_power_w,airspeed_multiplier,positive_climb_multiplier,pitch_rate_degrees_per_second,max_elevator_vertical_speed_mps,bank_rate_degrees_per_second,effective_mass_kg,glide_ratio,xr_base_z_m,xr_navigation_z_m,xr_viewer_z_m,x_m,y_m,heading_degrees,geospatial_navigation,longitude_degrees,latitude_degrees,ellipsoid_height_m,raw_steering_degrees,effective_steering_degrees,csc_wheel_revolutions,csc_wheel_event_time_ticks,csc_wheel_stopped,low_speed_coast_stopped,t2_control_status,t2_control_preset,ptt_held,voice_status,brake_button_held,trainer_grade_percent,steering_source\r\n"));
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
    const TArray<FString> Fields = {
        Timestamp,
        FString::Printf(TEXT("%.3f"), FPlatformTime::Seconds() - StartedAtSeconds),
        Event,
        FString::Printf(TEXT("%.2f"), Snapshot.SpeedKmh),
        FString::Printf(TEXT("%.2f"), FtmsSpeedKmh),
        FString::Printf(TEXT("%.1f"), Snapshot.CadenceRpm),
        FString::Printf(TEXT("%d"), Snapshot.PowerWatts),
        FString::Printf(TEXT("%.1f"), Snapshot.PropulsionPowerWatts),
        HeartRate,
        FString::Printf(TEXT("%.3f"), Snapshot.DistanceMeters),
        FString::Printf(TEXT("%d"), Snapshot.LapsCompleted),
        Snapshot.bFlightEnabled ? TEXT("1") : TEXT("0"),
        Snapshot.bAircraftAirborne ? TEXT("1") : TEXT("0"),
        Snapshot.bAircraftStalled ? TEXT("1") : TEXT("0"),
        Snapshot.bAircraftOverspeed ? TEXT("1") : TEXT("0"),
        FString::Printf(TEXT("%.3f"), Snapshot.AltitudeMeters),
        FString::Printf(TEXT("%.3f"), Snapshot.VerticalSpeedMetersPerSecond),
        FString::Printf(TEXT("%.3f"), Snapshot.FlightPathAngleDegrees),
        FString::Printf(TEXT("%.3f"), Snapshot.AngleOfAttackDegrees),
        FString::Printf(TEXT("%.3f"), Snapshot.FlightControlAuthority),
        FString::Printf(TEXT("%.3f"), Snapshot.BankDegrees),
        FString::Printf(TEXT("%.3f"), Snapshot.PitchDegrees),
        FString::Printf(TEXT("%.3f"), Snapshot.CommandedRollRightDegrees),
        FString::Printf(TEXT("%.3f"), Snapshot.CommandedPitchDegrees),
        Snapshot.bFlightTuningActive ? TEXT("1") : TEXT("0"),
        Snapshot.FlightTuningStatus,
        FString::Printf(TEXT("%.1f"), Snapshot.FlightTestPropulsionPowerWatts),
        FString::Printf(TEXT("%.2f"), Snapshot.FlightAirspeedMultiplier),
        FString::Printf(TEXT("%.2f"), Snapshot.FlightPositiveClimbMultiplier),
        FString::Printf(TEXT("%.1f"), Snapshot.FlightPitchRateDegreesPerSecond),
        FString::Printf(TEXT("%.2f"), Snapshot.FlightMaxElevatorVerticalSpeedMps),
        FString::Printf(TEXT("%.1f"), Snapshot.FlightBankRateDegreesPerSecond),
        FString::Printf(TEXT("%.1f"), Arrietty::FlightEffectiveMassKg),
        FString::Printf(TEXT("%.1f"), Arrietty::FlightGlideRatio),
        FString::Printf(TEXT("%.3f"), Arrietty::EyeHeightMeters),
        FString::Printf(TEXT("%.3f"), Snapshot.AltitudeMeters),
        FString::Printf(TEXT("%.3f"), EyeZ),
        FString::Printf(TEXT("%.3f"), Snapshot.PositionMeters.X),
        FString::Printf(TEXT("%.3f"), Snapshot.PositionMeters.Y),
        FString::Printf(TEXT("%.3f"), Snapshot.HeadingDegrees),
        Snapshot.bGeospatialNavigation ? TEXT("1") : TEXT("0"),
        FString::Printf(TEXT("%.8f"), Snapshot.LongitudeDegrees),
        FString::Printf(TEXT("%.8f"), Snapshot.LatitudeDegrees),
        FString::Printf(TEXT("%.3f"), Snapshot.EllipsoidHeightMeters),
        FString::Printf(TEXT("%.3f"), Snapshot.RawSteeringDegrees),
        FString::Printf(TEXT("%.3f"), Snapshot.EffectiveSteeringDegrees),
        WheelRevs,
        WheelTicks,
        bWheelStopped ? TEXT("1") : TEXT("0"),
        bLowSpeedCoastStopped ? TEXT("1") : TEXT("0"),
        Snapshot.ControlStatus,
        Preset,
        Snapshot.bPushToTalkHeld ? TEXT("1") : TEXT("0"),
        Snapshot.VoiceStatus,
        Snapshot.bBrakeButtonHeld ? TEXT("1") : TEXT("0"),
        FString::Printf(TEXT("%.1f"), Snapshot.AppliedGradePercent),
        Snapshot.SteeringSource
    };
    WriteUtf8(FString::Join(Fields, TEXT(",")) + TEXT("\r\n"));
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
