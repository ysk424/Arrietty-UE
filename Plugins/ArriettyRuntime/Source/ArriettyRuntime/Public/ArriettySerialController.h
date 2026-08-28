// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyControllerProtocol.h"
#include "Async/Future.h"
#include "Containers/Queue.h"

enum class EArriettyControllerEventType : uint8
{
    Status,
    Connected,
    Sample,
    Disconnected
};

struct FArriettyControllerEvent
{
    EArriettyControllerEventType Type = EArriettyControllerEventType::Status;
    FString Message;
    FArriettyControllerSample Sample;
};

class FArriettySerialController
{
public:
    FArriettySerialController() = default;
    ~FArriettySerialController();

    void Start();
    void RequestStop();
    void StopAndWait();
    bool DequeueEvent(FArriettyControllerEvent& OutEvent);

private:
    void WorkerMain();
    void QueueEvent(FArriettyControllerEvent&& Event);
    void QueueStatus(const FString& Message);

    TAtomic<bool> bStopRequested{false};
    TQueue<FArriettyControllerEvent, EQueueMode::Mpsc> Events;
    TFuture<void> WorkerFuture;
};
