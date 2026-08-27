// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "ArriettyAlertWidget.h"
#include "ArriettyInstrumentWidget.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArriettyNativeWidgetContentTest,
    "Arrietty.UI.Native Widget Content",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArriettyNativeWidgetContentTest::RunTest(const FString& Parameters)
{
    UArriettyInstrumentWidget* Instrument = NewObject<UArriettyInstrumentWidget>();
    TestNotNull(TEXT("Instrument widget is created"), Instrument);
    TSharedRef<SWidget> InstrumentSlate = Instrument->TakeWidget();
    InstrumentSlate->SlatePrepass(1.0f);
    TestNotNull(TEXT("Instrument UMG root exists before rendering"), Instrument->GetRootWidget());
    TestTrue(TEXT("Instrument Slate content is not the empty spacer"),
        InstrumentSlate->GetDesiredSize().X > 0.0f && InstrumentSlate->GetDesiredSize().Y > 0.0f);

    UArriettyAlertWidget* Alert = NewObject<UArriettyAlertWidget>();
    TestNotNull(TEXT("Alert widget is created"), Alert);
    TSharedRef<SWidget> AlertSlate = Alert->TakeWidget();
    AlertSlate->SlatePrepass(1.0f);
    TestNotNull(TEXT("Alert UMG root exists before rendering"), Alert->GetRootWidget());
    TestTrue(TEXT("Alert Slate content is not the empty spacer"),
        AlertSlate->GetDesiredSize().X > 0.0f && AlertSlate->GetDesiredSize().Y > 0.0f);

    return true;
}

#endif
