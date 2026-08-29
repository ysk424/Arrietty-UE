// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyControllerProtocol.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyControllerProtocolTest,
    "Arrietty.Controller.Serial Protocol",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyControllerProtocolTest::RunTest(const FString& Parameters)
{
    FArriettyControllerSample Sample;
    TestTrue(TEXT("Valid state parses"), ArriettyControllerProtocol::ParseStateLine(
        TEXT("A1,42,-32767,0,32767,1234,137"), Sample));
    TestEqual(TEXT("Sequence"), Sample.Sequence, static_cast<uint32>(42));
    TestEqual(TEXT("Joystick 1 X"), Sample.Joystick1X, -32767);
    TestEqual(TEXT("Joystick 1 Y"), Sample.Joystick1Y, 0);
    TestEqual(TEXT("Joystick 2 X"), Sample.Joystick2X, 32767);
    TestEqual(TEXT("Joystick 2 Y"), Sample.Joystick2Y, 1234);
    TestEqual(TEXT("Button mask"), Sample.ButtonMask, static_cast<uint8>(137));
    TestTrue(TEXT("Button 1 pressed"), ArriettyControllerProtocol::IsPressed(Sample.ButtonMask, 0));
    TestTrue(TEXT("Button 4 pressed"), ArriettyControllerProtocol::IsPressed(Sample.ButtonMask, 3));
    TestTrue(TEXT("Joystick 2 switch pressed"), ArriettyControllerProtocol::IsPressed(Sample.ButtonMask, 7));
    TestFalse(TEXT("Button 2 not pressed"), ArriettyControllerProtocol::IsPressed(Sample.ButtonMask, 1));
    TestTrue(TEXT("Button 5 power boost bit"), ArriettyControllerProtocol::IsPressed(0x10, 4));
    TestTrue(TEXT("Button 6 brake bit"), ArriettyControllerProtocol::IsPressed(0x20, 5));

    TestFalse(TEXT("Wrong version rejected"), ArriettyControllerProtocol::ParseStateLine(
        TEXT("A2,42,0,0,0,0,0"), Sample));
    TestFalse(TEXT("Missing field rejected"), ArriettyControllerProtocol::ParseStateLine(
        TEXT("A1,42,0,0,0,0"), Sample));
    TestFalse(TEXT("Axis overflow rejected"), ArriettyControllerProtocol::ParseStateLine(
        TEXT("A1,42,32768,0,0,0,0"), Sample));
    TestFalse(TEXT("Button overflow rejected"), ArriettyControllerProtocol::ParseStateLine(
        TEXT("A1,42,0,0,0,0,256"), Sample));
    TestFalse(TEXT("Non-numeric field rejected"), ArriettyControllerProtocol::ParseStateLine(
        TEXT("A1,not-a-number,0,0,0,0,0"), Sample));
    return true;
}

#endif
