// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

struct FArriettyControllerSample
{
    uint32 Sequence = 0;
    int32 Joystick1X = 0;
    int32 Joystick1Y = 0;
    int32 Joystick2X = 0;
    int32 Joystick2Y = 0;
    uint8 ButtonMask = 0;
    double ReceivedAtSeconds = 0.0;
};

namespace ArriettyControllerProtocol
{
bool ParseStateLine(const FString& Line, FArriettyControllerSample& OutSample);
bool IsPressed(uint8 ButtonMask, int32 BitIndex);
}
