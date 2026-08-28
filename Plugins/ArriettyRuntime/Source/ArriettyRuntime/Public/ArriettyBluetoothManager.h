// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyTypes.h"
#include "Async/Future.h"
#include "Containers/Queue.h"

enum class EArriettyBluetoothEventType : uint8
{
    Status,
    Connected,
    TrainerSample,
    CscSample,
    HeartRateConnected,
    HeartRateSample,
    HeartRateUnavailable,
    ControlReady,
    CscUnavailable,
    Error,
    WorkerStopped
};

struct FArriettyBluetoothEvent
{
    int32 Generation = 0;
    EArriettyBluetoothEventType Type = EArriettyBluetoothEventType::Status;
    EArriettyRideStatus Status = EArriettyRideStatus::Idle;
    FString Message;
    FArriettyTrainerSample TrainerSample;
    FArriettyCscSample CscSample;
    uint16 HeartRateBpm = 0;
    int32 PresetIndex = 0;
    double GradePercent = 0.0;
    double ReceivedAtSeconds = 0.0;
};

class FArriettyBluetoothManager
{
public:
    FArriettyBluetoothManager() = default;
    ~FArriettyBluetoothManager();

    void Start(int32 InitialPresetIndex, double InitialGradePercent = 0.0);
    void RequestStop();
    void StopAndWait();
    void RequestPreset(int32 PresetIndex);
    void RequestGrade(double GradePercent);
    bool DequeueEvent(FArriettyBluetoothEvent& OutEvent);
    bool IsRunning() const { return bWorkerRunning.Load(); }
    int32 GetGeneration() const { return Generation.Load(); }

private:
    void WorkerMain(int32 WorkerGeneration, int32 InitialPresetIndex, double InitialGradePercent);
    void QueueEvent(FArriettyBluetoothEvent&& Event);
    void QueueStatus(int32 WorkerGeneration, EArriettyRideStatus Status, const FString& Message);
    void QueueError(int32 WorkerGeneration, const FString& Message);

    TAtomic<bool> bStopRequested{false};
    TAtomic<bool> bWorkerRunning{false};
    TAtomic<int32> Generation{0};
    TQueue<FArriettyBluetoothEvent, EQueueMode::Mpsc> Events;
    struct FControlRequest
    {
        int32 Generation = 0;
        TOptional<int32> PresetIndex;
        TOptional<double> GradePercent;
    };
    TQueue<FControlRequest, EQueueMode::Mpsc> ControlRequests;
    TFuture<void> WorkerFuture;
};
