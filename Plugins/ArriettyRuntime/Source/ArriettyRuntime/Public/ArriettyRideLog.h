// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyTypes.h"

class FArchive;

class FArriettyRideLog
{
public:
    ~FArriettyRideLog();
    bool Start();
    void Record(
        const TCHAR* Event,
        const FArriettyRideSnapshot& Snapshot,
        double FtmsSpeedKmh,
        TOptional<uint32> WheelRevolutions,
        TOptional<uint16> WheelEventTicks,
        bool bWheelStopped,
        bool bLowSpeedCoastStopped);
    void Stop(const TCHAR* Event, const FArriettyRideSnapshot* Snapshot = nullptr);
    bool IsActive() const { return File != nullptr; }
    const FString& GetPath() const { return Path; }

private:
    void WriteUtf8(const FString& Text);
    TUniquePtr<FArchive> File;
    FString Path;
    double StartedAtSeconds = 0.0;
};
