// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyTrainerProtocol.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyFtmsParseTest,
    "Arrietty.Trainer.FTMS Indoor Bike Data",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyFtmsParseTest::RunTest(const FString& Parameters)
{
    const TArray<uint8> Data = {0x44, 0x00, 0xe0, 0x07, 0xb8, 0x00, 0xb0, 0x00};
    FArriettyTrainerSample Sample;
    TestTrue(TEXT("FTMS packet parses"), ArriettyTrainerProtocol::ParseIndoorBikeData(Data, Sample));
    TestTrue(TEXT("Speed present"), Sample.SpeedKmh.IsSet());
    TestTrue(TEXT("Cadence present"), Sample.CadenceRpm.IsSet());
    TestTrue(TEXT("Power present"), Sample.PowerWatts.IsSet());
    TestTrue(TEXT("Speed 20.16 km/h"), FMath::IsNearlyEqual(Sample.SpeedKmh.Get(0.0), 20.16));
    TestTrue(TEXT("Cadence 92 rpm"), FMath::IsNearlyEqual(Sample.CadenceRpm.Get(0.0), 92.0));
    TestEqual(TEXT("Power 176 W"), Sample.PowerWatts.Get(0), 176);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyCscParseTest,
    "Arrietty.Trainer.CSC Measurement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyCscParseTest::RunTest(const FString& Parameters)
{
    const TArray<uint8> Data = {0x03, 0x7d, 0x2a, 0x00, 0x00, 0xca, 0xa8, 0x59, 0x0c, 0xcb, 0x3a};
    FArriettyCscSample Sample;
    TestTrue(TEXT("CSC packet parses"), ArriettyTrainerProtocol::ParseCscMeasurement(Data, Sample));
    TestEqual(TEXT("Wheel revolutions"), Sample.WheelRevolutions.Get(0), static_cast<uint32>(10877));
    TestEqual(TEXT("Wheel event ticks"), Sample.WheelEventTimeTicks.Get(0), static_cast<uint16>(0xa8ca));
    TestEqual(TEXT("Crank revolutions"), Sample.CrankRevolutions.Get(0), static_cast<uint16>(0x0c59));
    TestEqual(TEXT("Crank event ticks"), Sample.CrankEventTimeTicks.Get(0), static_cast<uint16>(0x3acb));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyHeartRateParseTest,
    "Arrietty.Sensors.Heart Rate Measurement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyHeartRateParseTest::RunTest(const FString& Parameters)
{
    const TArray<uint8> EightBit = {0x00, 72};
    const TArray<uint8> SixteenBit = {0x01, 0x2c, 0x01};
    const TArray<uint8> TruncatedSixteenBit = {0x01, 0x2c};
    TestEqual(TEXT("UINT8 heart rate"),
        ArriettyTrainerProtocol::ParseHeartRateMeasurement(EightBit).Get(0),
        static_cast<uint16>(72));
    TestEqual(TEXT("UINT16 heart rate"),
        ArriettyTrainerProtocol::ParseHeartRateMeasurement(SixteenBit).Get(0),
        static_cast<uint16>(300));
    TestFalse(TEXT("Truncated UINT16 is rejected"),
        ArriettyTrainerProtocol::ParseHeartRateMeasurement(TruncatedSixteenBit).IsSet());
    TestFalse(TEXT("Empty measurement is rejected"),
        ArriettyTrainerProtocol::ParseHeartRateMeasurement({}).IsSet());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyControlCommandsTest,
    "Arrietty.Trainer.Flat Road Control",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyControlCommandsTest::RunTest(const FString& Parameters)
{
    const TArray<TArray<uint8>> Expected = {
        {0x11, 0, 0, 0, 0, 0x28, 0x33},
        {0x11, 0, 0, 0, 0, 0x50, 0x33},
        {0x11, 0, 0, 0, 0, 0x78, 0x33},
        {0x11, 0, 0, 0, 0, 0xa0, 0x33},
        {0x11, 0, 0, 0, 0, 0xc8, 0x33},
        {0x11, 0, 0, 0, 0, 0xf0, 0x33},
        {0x11, 0, 0, 0, 0, 0xff, 0x33},
    };
    for (int32 PresetIndex = 1; PresetIndex <= 7; ++PresetIndex)
    {
        TestEqual(
            *FString::Printf(TEXT("P%d command"), PresetIndex),
            ArriettyTrainerProtocol::BuildFlatRoadControlCommand(PresetIndex),
            Expected[PresetIndex - 1]);
    }
    TestEqual(
        TEXT("P5 brake grade 3 percent"),
        ArriettyTrainerProtocol::BuildSimulationControlCommand(5, 3.0),
        TArray<uint8>({0x11, 0, 0, 0x2c, 0x01, 0xc8, 0x33}));
    TestEqual(
        TEXT("P5 released brake returns to grade zero"),
        ArriettyTrainerProtocol::BuildSimulationControlCommand(5, 0.0),
        Expected[4]);
    const TArray<uint8> Success = {0x80, 0x00, 0x01};
    const TArray<uint8> OtherOpcode = {0x80, 0x11, 0x01};
    TestEqual(TEXT("Control success"), ArriettyTrainerProtocol::ParseControlResponse(Success, 0x00).Get(0), static_cast<uint8>(1));
    TestFalse(TEXT("Unrelated response ignored"), ArriettyTrainerProtocol::ParseControlResponse(OtherOpcode, 0x00).IsSet());
    TestEqual(TEXT("Result name"), ArriettyTrainerProtocol::ControlResultName(0x05), FString(TEXT("control not permitted")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyRideMathTest,
    "Arrietty.Ride Movement Rules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyRideMathTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Adaptive wheel timeout"), FMath::IsNearlyEqual(
        ArriettyTrainerProtocol::WheelStopTimeoutSeconds(1.0), 1.75));
    TestEqual(TEXT("Stale speed stops"), ArriettyTrainerProtocol::EffectiveSpeedKmh(
        101.3, 100.0, 20.0, 80.0, false, 0.0, 0.0), 0.0);
    TestEqual(TEXT("Low-speed coasting stops"), ArriettyTrainerProtocol::EffectiveSpeedKmh(
        100.1, 100.0, 5.0, 0.0, false, 0.0, 0.0), 0.0);
    TestEqual(TEXT("Low-speed pedaling moves"), ArriettyTrainerProtocol::EffectiveSpeedKmh(
        100.1, 100.0, 5.0, 12.0, false, 0.0, 0.0), 5.0);
    TestEqual(TEXT("Stationary CSC wheel stops"), ArriettyTrainerProtocol::EffectiveSpeedKmh(
        101.751, 100.8, 15.0, 80.0, true, 100.0, 1.0), 0.0);
    TestEqual(TEXT("20 km/h flight altitude"), ArriettyTrainerProtocol::AltitudeForSpeed(20.0), 10.0);
    TestEqual(TEXT("Below takeoff is ground"), ArriettyTrainerProtocol::AltitudeForSpeed(10.0), 0.0);
    TestTrue(TEXT("Ground mode requires ride surface"), ArriettyTrainerProtocol::RequiresRideSurface(false));
    TestFalse(TEXT("Flight mode can leave ride surface"), ArriettyTrainerProtocol::RequiresRideSurface(true));
    TestEqual(TEXT("Four completed laps"), ArriettyTrainerProtocol::CompletedLaps(572.0, 143.0), 4);
    TestTrue(TEXT("Steering deadzone/gain"), FMath::IsNearlyEqual(
        ArriettyTrainerProtocol::EffectiveSteeringDegrees(20.0), 9.25));
    TestEqual(TEXT("Steering clamp"), ArriettyTrainerProtocol::EffectiveSteeringDegrees(60.0), 15.0);
    TestEqual(TEXT("Unreal +X is ride heading 0"),
        ArriettyTrainerProtocol::HeadingDegreesForUnrealWorldForward(FVector2D(1.0, 0.0)), 0.0);
    TestEqual(TEXT("Unreal +Y is ride heading -90"),
        ArriettyTrainerProtocol::HeadingDegreesForUnrealWorldForward(FVector2D(0.0, 1.0)), -90.0);
    TestTrue(TEXT("Unreal X=Y is ride heading -45"), FMath::IsNearlyEqual(
        ArriettyTrainerProtocol::HeadingDegreesForUnrealWorldForward(FVector2D(1.0, 1.0)), -45.0));
    TestEqual(TEXT("View +X rotates 90 degrees to course +Y"),
        ArriettyTrainerProtocol::YawCorrectionDegrees(FVector2D(1.0, 0.0), FVector2D(0.0, 1.0)),
        90.0);
    TestEqual(TEXT("View +Y rotates -90 degrees to course +X"),
        ArriettyTrainerProtocol::YawCorrectionDegrees(FVector2D(0.0, 1.0), FVector2D(1.0, 0.0)),
        -90.0);
    TestEqual(TEXT("Aligned view needs no correction"),
        ArriettyTrainerProtocol::YawCorrectionDegrees(FVector2D(1.0, 0.0), FVector2D(1.0, 0.0)),
        0.0);
    return true;
}

#endif
