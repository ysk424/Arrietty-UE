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

    int32 GetRequestedLevel() const { return RequestedLevel; }
    int32 GetReportedLevel() const { return ReportedLevel; }
    const FString& GetStatus() const { return Status; }
    static int32 LevelForSpeed(double SpeedKmh);
    static TOptional<int32> ParseResponseLevel(const FString& Response);

private:
    bool SendCommand(const FString& Command);
    void PollResponses(double NowSeconds);

    FSocket* Socket = nullptr;
    TSharedPtr<FInternetAddr> Destination;
    int32 LastRequestedLevel = INDEX_NONE;
    int32 RequestedLevel = 0;
    int32 ReportedLevel = INDEX_NONE;
    double LastSendSeconds = -1.0;
    double LastResponseSeconds = -1.0;
    double FirstUnansweredSendSeconds = -1.0;
    FString Status = TEXT("NOT STARTED");
};
