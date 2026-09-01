// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "ArriettyFanController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyFanSpeedMappingTest,
    "Arrietty.Hardware Fan Speed Mapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyFanSpeedMappingTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Stopped"), FArriettyFanController::LevelForSpeed(0.0), 0);
    TestEqual(TEXT("Below stopped threshold"), FArriettyFanController::LevelForSpeed(0.5), 0);
    TestEqual(TEXT("Low speed"), FArriettyFanController::LevelForSpeed(0.6), 1);
    TestEqual(TEXT("Ten km/h"), FArriettyFanController::LevelForSpeed(10.0), 2);
    TestEqual(TEXT("Twenty km/h"), FArriettyFanController::LevelForSpeed(20.0), 4);
    TestEqual(TEXT("Maximum speed"), FArriettyFanController::LevelForSpeed(30.0), 6);
    TestEqual(TEXT("Above maximum speed"), FArriettyFanController::LevelForSpeed(50.0), 6);
    TestEqual(TEXT("Level response"),
        FArriettyFanController::ParseResponseLevel(TEXT("OK LEVEL 4")).Get(-1), 4);
    TestEqual(TEXT("Status response"),
        FArriettyFanController::ParseResponseLevel(TEXT("OK LEVEL 2 TARGET 5")).Get(-1), 2);
    TestEqual(TEXT("Sync response"),
        FArriettyFanController::ParseResponseLevel(TEXT("OK SYNC 3")).Get(-1), 3);
    TestFalse(TEXT("Error response rejected"),
        FArriettyFanController::ParseResponseLevel(TEXT("ERR unknown")).IsSet());
    TestFalse(TEXT("Out-of-range response rejected"),
        FArriettyFanController::ParseResponseLevel(TEXT("OK LEVEL 7")).IsSet());
    return true;
}

#endif
