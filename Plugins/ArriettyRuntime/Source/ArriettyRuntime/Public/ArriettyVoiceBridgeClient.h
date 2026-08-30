// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

class FSocket;
class FInternetAddr;

/** Sends Button 5 PTT edges to the local Windows voice bridge over UDP. */
class FArriettyVoiceBridgeClient
{
public:
    FArriettyVoiceBridgeClient();
    ~FArriettyVoiceBridgeClient();

    bool IsAvailable() const;
    bool SendPttDown();
    bool SendPttUp();
    bool SendPttCancel();
    bool PollStatus(FString& OutStatus, FString& OutDetail);

private:
    bool SendCommand(const ANSICHAR* Command);

    FSocket* Socket = nullptr;
    TSharedPtr<FInternetAddr> Destination;
};
