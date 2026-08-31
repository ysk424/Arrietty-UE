// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyControllerProtocol.h"
#include "ArriettyDigitalFlightControls.h"
#include "ArriettyFlightTuningControls.h"

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
    TestTrue(TEXT("Button 5 PTT bit"), ArriettyControllerProtocol::IsPressed(0x10, 4));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyDigitalFlightControlsTest,
    "Arrietty.Controller.Digital Flight Controls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyDigitalFlightControlsTest::RunTest(const FString& Parameters)
{
    FArriettyDigitalFlightControls Controls;
    Controls.Reset();

    Controls.UpdateJoystick(FVector2D(0.80, 0.0));
    TestEqual(TEXT("First pull adds one pitch degree"), Controls.GetPitchDegrees(), 1.0);
    Controls.UpdateJoystick(FVector2D(1.0, 0.0));
    TestEqual(TEXT("Holding pitch does not repeat"), Controls.GetPitchDegrees(), 1.0);
    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(0.46, 0.0));
    TestEqual(TEXT("Returning to center rearms pitch"), Controls.GetPitchDegrees(), 2.0);

    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(0.0, -0.80));
    TestEqual(TEXT("Installed J2 right adds one right-roll degree"),
        Controls.GetRollRightDegrees(), 1.0);
    TestTrue(TEXT("Right-roll command maps to the existing right-bank aileron sign"),
        Controls.GetAileronInput() < 0.0);
    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(0.0, 0.80));
    TestEqual(TEXT("J2 left subtracts one right-roll degree"),
        Controls.GetRollRightDegrees(), 0.0);

    Controls.StepRollRight(-1);
    Controls.StepPitch(-1);
    Controls.ResetCommands(FVector2D::ZeroVector);
    TestEqual(TEXT("J2 center button resets roll"), Controls.GetRollRightDegrees(), 0.0);
    TestEqual(TEXT("J2 center button resets pitch"), Controls.GetPitchDegrees(), 0.0);

    for (int32 Index = 0; Index < 100; ++Index)
    {
        Controls.StepPitch(1);
        Controls.StepRollRight(1);
    }
    TestEqual(TEXT("Pitch command clamps at the aircraft limit"),
        Controls.GetPitchDegrees(), Arrietty::FlightMaxPitchDegrees);
    TestEqual(TEXT("Roll command clamps at the aircraft limit"),
        Controls.GetRollRightDegrees(), Arrietty::FlightMaxBankDegrees);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyFlightTuningControlsTest,
    "Arrietty.Controller.Flight Tuning Controls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyFlightTuningControlsTest::RunTest(const FString& Parameters)
{
    FArriettyFlightTuningControls Controls;
    Controls.Reset();
    Controls.UpdateJoystick(FVector2D(0.8, 0.0));
    TestFalse(TEXT("J1 X does nothing before tuning starts"), Controls.IsActive());
    TestEqual(TEXT("Default test propulsion is 95 W"),
        Controls.GetValues().TestPropulsionPowerWatts, 95.0);
    TestEqual(TEXT("Default flight speed multiplier is three"),
        Controls.GetValues().AirspeedMultiplier, 3.0);
    TestEqual(TEXT("Default positive climb multiplier is ten"),
        Controls.GetValues().PositiveClimbMultiplier, 10.0);
    TestEqual(TEXT("Flight tuning exposes six items"), Controls.GetParameterCount(), 6);

    const FArriettyFlightTuningChange Entered = Controls.PressSwitch();
    TestTrue(TEXT("J1 SW starts tuning"), Entered.bEntered && Controls.IsActive());
    TestEqual(TEXT("First item is test propulsion"),
        Controls.GetParameter(), EArriettyFlightTuningParameter::TestPropulsionPower);
    Controls.UpdateJoystick(FVector2D(0.8, 0.0));
    TestEqual(TEXT("J1 right adds 5 W"),
        Controls.GetValues().TestPropulsionPowerWatts, 100.0);
    Controls.UpdateJoystick(FVector2D(1.0, 0.0));
    TestEqual(TEXT("Holding J1 right does not repeat"),
        Controls.GetValues().TestPropulsionPowerWatts, 100.0);
    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(-0.8, 0.0));
    TestEqual(TEXT("J1 left subtracts 5 W after returning to center"),
        Controls.GetValues().TestPropulsionPowerWatts, 95.0);

    Controls.PressSwitch();
    TestEqual(TEXT("Second item is flight speed multiplier"),
        Controls.GetParameter(), EArriettyFlightTuningParameter::AirspeedMultiplier);
    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(0.8, 0.0));
    TestEqual(TEXT("Flight speed multiplier changes by 0.5"),
        Controls.GetValues().AirspeedMultiplier, 3.5);

    Controls.PressSwitch();
    TestEqual(TEXT("Third item is positive climb multiplier"),
        Controls.GetParameter(), EArriettyFlightTuningParameter::PositiveClimbMultiplier);
    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(0.8, 0.0));
    TestEqual(TEXT("Positive climb multiplier changes by one"),
        Controls.GetValues().PositiveClimbMultiplier, 11.0);

    Controls.PressSwitch();
    TestEqual(TEXT("Fourth item is pitch response"),
        Controls.GetParameter(), EArriettyFlightTuningParameter::PitchResponseRate);
    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(0.8, 0.0));
    TestEqual(TEXT("Pitch response changes by 2 degrees per second"),
        Controls.GetValues().PitchRateDegreesPerSecond, 30.0);

    Controls.PressSwitch();
    TestEqual(TEXT("Fifth item is elevator vertical speed"),
        Controls.GetParameter(), EArriettyFlightTuningParameter::ElevatorVerticalSpeed);
    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(0.8, 0.0));
    TestTrue(TEXT("Vertical speed changes by 0.1 m/s"), FMath::IsNearlyEqual(
        Controls.GetValues().MaxElevatorVerticalSpeedMps, 1.6));

    Controls.PressSwitch();
    TestEqual(TEXT("Sixth item is roll response"),
        Controls.GetParameter(), EArriettyFlightTuningParameter::BankResponseRate);
    Controls.UpdateJoystick(FVector2D::ZeroVector);
    Controls.UpdateJoystick(FVector2D(-0.8, 0.0));
    TestEqual(TEXT("Roll response changes by 5 degrees per second"),
        Controls.GetValues().BankRateDegreesPerSecond, 50.0);
    const FArriettyFlightTuningChange Completed = Controls.PressSwitch();
    TestTrue(TEXT("Confirming the sixth item completes tuning"),
        Completed.bCompleted && !Controls.IsActive());
    return true;
}

#endif
