// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyControllerProtocol.h"

#include "Misc/CString.h"

namespace
{
bool TryParseSigned(const FString& Text, int32& OutValue)
{
    if (Text.IsEmpty())
    {
        return false;
    }
    TCHAR* End = nullptr;
    const int64 Value = FCString::Strtoi64(*Text, &End, 10);
    if (End == *Text || *End != TEXT('\0') || Value < MIN_int32 || Value > MAX_int32)
    {
        return false;
    }
    OutValue = static_cast<int32>(Value);
    return true;
}

bool TryParseSequence(const FString& Text, uint32& OutValue)
{
    if (Text.IsEmpty() || Text.StartsWith(TEXT("-")))
    {
        return false;
    }
    TCHAR* End = nullptr;
    const uint64 Value = FCString::Strtoui64(*Text, &End, 10);
    if (End == *Text || *End != TEXT('\0') || Value > MAX_uint32)
    {
        return false;
    }
    OutValue = static_cast<uint32>(Value);
    return true;
}
}

bool ArriettyControllerProtocol::ParseStateLine(
    const FString& Line,
    FArriettyControllerSample& OutSample)
{
    TArray<FString> Parts;
    Line.ParseIntoArray(Parts, TEXT(","), false);
    if (Parts.Num() != 7 || Parts[0] != TEXT("A1"))
    {
        return false;
    }

    uint32 Sequence = 0;
    int32 Joystick1X = 0;
    int32 Joystick1Y = 0;
    int32 Joystick2X = 0;
    int32 Joystick2Y = 0;
    int32 ButtonMask = 0;
    if (!TryParseSequence(Parts[1], Sequence) ||
        !TryParseSigned(Parts[2], Joystick1X) ||
        !TryParseSigned(Parts[3], Joystick1Y) ||
        !TryParseSigned(Parts[4], Joystick2X) ||
        !TryParseSigned(Parts[5], Joystick2Y) ||
        !TryParseSigned(Parts[6], ButtonMask))
    {
        return false;
    }

    const auto AxisIsValid = [](int32 Value)
    {
        return Value >= -32767 && Value <= 32767;
    };
    if (!AxisIsValid(Joystick1X) ||
        !AxisIsValid(Joystick1Y) ||
        !AxisIsValid(Joystick2X) ||
        !AxisIsValid(Joystick2Y) ||
        ButtonMask < 0 || ButtonMask > 255)
    {
        return false;
    }

    OutSample.Sequence = Sequence;
    OutSample.Joystick1X = Joystick1X;
    OutSample.Joystick1Y = Joystick1Y;
    OutSample.Joystick2X = Joystick2X;
    OutSample.Joystick2Y = Joystick2Y;
    OutSample.ButtonMask = static_cast<uint8>(ButtonMask);
    return true;
}

bool ArriettyControllerProtocol::IsPressed(uint8 ButtonMask, int32 BitIndex)
{
    return BitIndex >= 0 && BitIndex < 8 &&
        (ButtonMask & static_cast<uint8>(1u << BitIndex)) != 0;
}
