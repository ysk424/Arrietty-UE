// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

class FInternetAddr;
class FSocket;

class FArriettyFanController
{
public:
    FArriettyFanController() = default;
    ~FArriettyFanController();

    bool Start();
    void Stop();
    void Tick(double SpeedKmh, double NowSeconds);
    void CorrectReportedLevel(int32 Delta);

    int32 GetReportedLevel() const { return ReportedLevel; }
    static int32 LevelForSpeed(double SpeedKmh);

private:
    bool SendCommand(const FString& Command);

    FSocket* Socket = nullptr;
    TSharedPtr<FInternetAddr> Destination;
    int32 LastRequestedLevel = INDEX_NONE;
    int32 ReportedLevel = 0;
    double LastSendSeconds = -1.0;
};
