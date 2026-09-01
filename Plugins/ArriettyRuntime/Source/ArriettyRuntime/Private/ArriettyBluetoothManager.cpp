// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyBluetoothManager.h"

#include "ArriettyTrainerProtocol.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"

#include <chrono>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cwctype>
#include <deque>
#include <mutex>
#include <string>
#include <stdexcept>
#include <vector>

#if ARRIETTY_WINDOWS_BLE
#include "Windows/AllowWindowsPlatformTypes.h"
#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Security.Cryptography.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogArriettyBluetooth, Log, All);

FArriettyBluetoothManager::~FArriettyBluetoothManager()
{
    StopAndWait();
}

void FArriettyBluetoothManager::Start(int32 InitialPresetIndex, double InitialGradePercent)
{
    StopAndWait();
    FArriettyBluetoothEvent Discarded;
    while (Events.Dequeue(Discarded))
    {
    }
    FControlRequest DiscardedRequest;
    while (ControlRequests.Dequeue(DiscardedRequest))
    {
    }

    const int32 WorkerGeneration = Generation.Load() + 1;
    Generation.Store(WorkerGeneration);
    bStopRequested.Store(false);
    bWorkerRunning.Store(true);
    WorkerFuture = Async(EAsyncExecution::Thread, [this, WorkerGeneration, InitialPresetIndex, InitialGradePercent]
    {
        WorkerMain(WorkerGeneration, InitialPresetIndex, InitialGradePercent);
    });
}

void FArriettyBluetoothManager::RequestStop()
{
    bStopRequested.Store(true);
}

void FArriettyBluetoothManager::StopAndWait()
{
    RequestStop();
    if (WorkerFuture.IsValid())
    {
        WorkerFuture.Wait();
        WorkerFuture = TFuture<void>();
    }
    bWorkerRunning.Store(false);
}

void FArriettyBluetoothManager::RequestPreset(int32 PresetIndex)
{
    FControlRequest Request;
    Request.Generation = Generation.Load();
    Request.PresetIndex = PresetIndex;
    ControlRequests.Enqueue(MoveTemp(Request));
}

void FArriettyBluetoothManager::RequestGrade(double GradePercent)
{
    FControlRequest Request;
    Request.Generation = Generation.Load();
    Request.GradePercent = GradePercent;
    ControlRequests.Enqueue(MoveTemp(Request));
}

bool FArriettyBluetoothManager::DequeueEvent(FArriettyBluetoothEvent& OutEvent)
{
    return Events.Dequeue(OutEvent);
}

void FArriettyBluetoothManager::QueueEvent(FArriettyBluetoothEvent&& Event)
{
    Events.Enqueue(MoveTemp(Event));
}

void FArriettyBluetoothManager::QueueStatus(
    int32 WorkerGeneration,
    EArriettyRideStatus Status,
    const FString& Message)
{
    FArriettyBluetoothEvent Event;
    Event.Generation = WorkerGeneration;
    Event.Type = EArriettyBluetoothEventType::Status;
    Event.Status = Status;
    Event.Message = Message;
    QueueEvent(MoveTemp(Event));
}

void FArriettyBluetoothManager::QueueError(int32 WorkerGeneration, const FString& Message)
{
    FArriettyBluetoothEvent Event;
    Event.Generation = WorkerGeneration;
    Event.Type = EArriettyBluetoothEventType::Error;
    Event.Status = EArriettyRideStatus::Error;
    Event.Message = Message;
    QueueEvent(MoveTemp(Event));
}

void FArriettyBluetoothManager::WorkerMain(
    int32 WorkerGeneration,
    int32 InitialPresetIndex,
    double InitialGradePercent)
{
#if !ARRIETTY_WINDOWS_BLE
    QueueError(WorkerGeneration, TEXT("CYCPLUS T2 support currently requires Windows"));
#else
    using namespace winrt;
    using namespace winrt::Windows::Devices::Bluetooth;
    using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
    using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;
    using namespace winrt::Windows::Security::Cryptography;

    const auto BluetoothUuid = [](uint16 ShortUuid)
    {
        return winrt::guid{
            static_cast<uint32>(ShortUuid), 0x0000, 0x1000,
            {0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb}};
    };
    const winrt::guid FtmsServiceUuid = BluetoothUuid(0x1826);
    const winrt::guid CscServiceUuid = BluetoothUuid(0x1816);
    const winrt::guid IndoorBikeDataUuid = BluetoothUuid(0x2ad2);
    const winrt::guid ControlPointUuid = BluetoothUuid(0x2ad9);
    const winrt::guid CscMeasurementUuid = BluetoothUuid(0x2a5b);
    const winrt::guid HeartRateServiceUuid = BluetoothUuid(0x180d);
    const winrt::guid HeartRateMeasurementUuid = BluetoothUuid(0x2a37);

    bool bApartmentInitialized = false;
    try
    {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        bApartmentInitialized = true;
        QueueStatus(
            WorkerGeneration,
            EArriettyRideStatus::Searching,
            TEXT("Searching for CYCPLUS T2 and BLE heart-rate sensor"));

        std::mutex ScanMutex;
        std::condition_variable ScanCondition;
        uint64 DeviceAddress = 0;
        uint64 HeartRateDeviceAddress = 0;
        BluetoothLEAdvertisementWatcher Watcher;
        Watcher.ScanningMode(BluetoothLEScanningMode::Active);
        const event_token ScanToken = Watcher.Received(
            [&](BluetoothLEAdvertisementWatcher const&, BluetoothLEAdvertisementReceivedEventArgs const& Args)
            {
                const std::wstring Name(Args.Advertisement().LocalName().c_str());
                std::wstring LowerName = Name;
                std::transform(LowerName.begin(), LowerName.end(), LowerName.begin(), ::towlower);
                const bool bIsT2 = LowerName.find(L"t2") != std::wstring::npos;
                bool bAdvertisesHeartRate = false;
                for (const winrt::guid& ServiceUuid : Args.Advertisement().ServiceUuids())
                {
                    if (ServiceUuid == HeartRateServiceUuid)
                    {
                        bAdvertisesHeartRate = true;
                        break;
                    }
                }
                if (!bIsT2 && !bAdvertisesHeartRate)
                {
                    return;
                }

                {
                    std::lock_guard Lock(ScanMutex);
                    if (bIsT2)
                    {
                        DeviceAddress = Args.BluetoothAddress();
                    }
                    if (bAdvertisesHeartRate)
                    {
                        HeartRateDeviceAddress = Args.BluetoothAddress();
                    }
                }
                ScanCondition.notify_all();
            });
        Watcher.Start();

        const auto ScanDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        {
            std::unique_lock Lock(ScanMutex);
            while (DeviceAddress == 0 && !bStopRequested.Load())
            {
                if (std::chrono::steady_clock::now() >= ScanDeadline)
                {
                    break;
                }
                ScanCondition.wait_for(Lock, std::chrono::milliseconds(100));
            }
        }
        Watcher.Stop();
        Watcher.Received(ScanToken);
        if (bStopRequested.Load())
        {
            throw std::runtime_error("stopped");
        }
        if (DeviceAddress == 0)
        {
            throw std::runtime_error("T2 was not found. Pedal several times, then press Numpad 0 again");
        }

        QueueStatus(WorkerGeneration, EArriettyRideStatus::Connecting, TEXT("Connecting to CYCPLUS T2"));
        BluetoothLEDevice Device = BluetoothLEDevice::FromBluetoothAddressAsync(DeviceAddress).get();
        if (!Device)
        {
            throw std::runtime_error("Windows could not open the CYCPLUS T2 Bluetooth device");
        }

        const auto RequireService = [&](const winrt::guid& Uuid, const char* Name)
        {
            const auto Result = Device.GetGattServicesForUuidAsync(Uuid, BluetoothCacheMode::Uncached).get();
            if (Result.Status() != GattCommunicationStatus::Success || Result.Services().Size() == 0)
            {
                throw std::runtime_error(std::string("T2 does not expose ") + Name);
            }
            return Result.Services().GetAt(0);
        };
        const auto RequireCharacteristic = [](const GattDeviceService& Service, const winrt::guid& Uuid, const char* Name)
        {
            const auto Result = Service.GetCharacteristicsForUuidAsync(Uuid, BluetoothCacheMode::Uncached).get();
            if (Result.Status() != GattCommunicationStatus::Success || Result.Characteristics().Size() == 0)
            {
                throw std::runtime_error(std::string("T2 does not expose ") + Name);
            }
            return Result.Characteristics().GetAt(0);
        };

        GattDeviceService FtmsService = RequireService(FtmsServiceUuid, "FTMS service 0x1826");
        GattCharacteristic IndoorBikeData = RequireCharacteristic(FtmsService, IndoorBikeDataUuid, "Indoor Bike Data 0x2AD2");
        GattCharacteristic ControlPoint = RequireCharacteristic(FtmsService, ControlPointUuid, "FTMS Control Point 0x2AD9");

        std::mutex ControlMutex;
        std::condition_variable ControlCondition;
        std::deque<std::vector<uint8>> ControlResponses;
        const event_token ControlToken = ControlPoint.ValueChanged(
            [&](GattCharacteristic const&, GattValueChangedEventArgs const& Args)
            {
                winrt::com_array<uint8_t> Bytes;
                CryptographicBuffer::CopyToByteArray(Args.CharacteristicValue(), Bytes);
                {
                    std::lock_guard Lock(ControlMutex);
                    ControlResponses.emplace_back(Bytes.begin(), Bytes.end());
                }
                ControlCondition.notify_all();
            });
        if (ControlPoint.WriteClientCharacteristicConfigurationDescriptorAsync(
                GattClientCharacteristicConfigurationDescriptorValue::Indicate).get()
            != GattCommunicationStatus::Success)
        {
            throw std::runtime_error("Could not subscribe to the T2 FTMS Control Point");
        }

        const auto SendControlCommand = [&](const TArray<uint8>& Command)
        {
            if (Command.IsEmpty())
            {
                throw std::runtime_error("Invalid T2 control preset");
            }
            const uint8 RequestedOpcode = Command[0];
            const auto Buffer = CryptographicBuffer::CreateFromByteArray(
                winrt::array_view<const uint8_t>(Command.GetData(), Command.GetData() + Command.Num()));
            if (ControlPoint.WriteValueAsync(Buffer, GattWriteOption::WriteWithResponse).get()
                != GattCommunicationStatus::Success)
            {
                throw std::runtime_error("Windows could not write the T2 FTMS Control Point");
            }

            const auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            std::unique_lock Lock(ControlMutex);
            while (!bStopRequested.Load())
            {
                while (!ControlResponses.empty())
                {
                    const std::vector<uint8> Response = std::move(ControlResponses.front());
                    ControlResponses.pop_front();
                    const TOptional<uint8> Result = ArriettyTrainerProtocol::ParseControlResponse(
                        MakeArrayView(Response.data(), static_cast<int32>(Response.size())),
                        RequestedOpcode);
                    if (!Result.IsSet())
                    {
                        continue;
                    }
                    if (Result.GetValue() != 0x01)
                    {
                        throw std::runtime_error(TCHAR_TO_UTF8(
                            *FString::Printf(
                                TEXT("T2 rejected FTMS opcode 0x%02x: %s"),
                                RequestedOpcode,
                                *ArriettyTrainerProtocol::ControlResultName(Result.GetValue()))));
                    }
                    return;
                }
                if (std::chrono::steady_clock::now() >= Deadline)
                {
                    throw std::runtime_error("T2 did not answer the FTMS control command");
                }
                ControlCondition.wait_for(Lock, std::chrono::milliseconds(100));
            }
            throw std::runtime_error("stopped");
        };

        SendControlCommand(TArray<uint8>{0x00});
        int32 CurrentPresetIndex = InitialPresetIndex;
        double CurrentGradePercent = InitialGradePercent;
        SendControlCommand(ArriettyTrainerProtocol::BuildSimulationControlCommand(
            CurrentPresetIndex, CurrentGradePercent));
        {
            FArriettyBluetoothEvent Event;
            Event.Generation = WorkerGeneration;
            Event.Type = EArriettyBluetoothEventType::ControlReady;
            Event.PresetIndex = CurrentPresetIndex;
            Event.GradePercent = CurrentGradePercent;
            QueueEvent(MoveTemp(Event));
        }

        const event_token TrainerToken = IndoorBikeData.ValueChanged(
            [this, WorkerGeneration](GattCharacteristic const&, GattValueChangedEventArgs const& Args)
            {
                winrt::com_array<uint8_t> Bytes;
                CryptographicBuffer::CopyToByteArray(Args.CharacteristicValue(), Bytes);
                FArriettyTrainerSample Sample;
                if (!ArriettyTrainerProtocol::ParseIndoorBikeData(
                        MakeArrayView(Bytes.data(), static_cast<int32>(Bytes.size())), Sample))
                {
                    return;
                }
                FArriettyBluetoothEvent Event;
                Event.Generation = WorkerGeneration;
                Event.Type = EArriettyBluetoothEventType::TrainerSample;
                Event.TrainerSample = MoveTemp(Sample);
                Event.ReceivedAtSeconds = FPlatformTime::Seconds();
                QueueEvent(MoveTemp(Event));
            });
        if (IndoorBikeData.WriteClientCharacteristicConfigurationDescriptorAsync(
                GattClientCharacteristicConfigurationDescriptorValue::Notify).get()
            != GattCommunicationStatus::Success)
        {
            throw std::runtime_error("Could not subscribe to T2 Indoor Bike Data");
        }

        GattDeviceService CscService{nullptr};
        GattCharacteristic CscMeasurement{nullptr};
        event_token CscToken{};
        bool bCscEnabled = false;
        try
        {
            CscService = RequireService(CscServiceUuid, "CSC service 0x1816");
            CscMeasurement = RequireCharacteristic(CscService, CscMeasurementUuid, "CSC Measurement 0x2A5B");
            CscToken = CscMeasurement.ValueChanged(
                [this, WorkerGeneration](GattCharacteristic const&, GattValueChangedEventArgs const& Args)
                {
                    winrt::com_array<uint8_t> Bytes;
                    CryptographicBuffer::CopyToByteArray(Args.CharacteristicValue(), Bytes);
                    FArriettyCscSample Sample;
                    if (!ArriettyTrainerProtocol::ParseCscMeasurement(
                            MakeArrayView(Bytes.data(), static_cast<int32>(Bytes.size())), Sample))
                    {
                        return;
                    }
                    FArriettyBluetoothEvent Event;
                    Event.Generation = WorkerGeneration;
                    Event.Type = EArriettyBluetoothEventType::CscSample;
                    Event.CscSample = MoveTemp(Sample);
                    Event.ReceivedAtSeconds = FPlatformTime::Seconds();
                    QueueEvent(MoveTemp(Event));
                });
            bCscEnabled = CscMeasurement.WriteClientCharacteristicConfigurationDescriptorAsync(
                GattClientCharacteristicConfigurationDescriptorValue::Notify).get()
                == GattCommunicationStatus::Success;
        }
        catch (const winrt::hresult_error& Error)
        {
            FArriettyBluetoothEvent Event;
            Event.Generation = WorkerGeneration;
            Event.Type = EArriettyBluetoothEventType::CscUnavailable;
            Event.Message = FString::Printf(TEXT("CSC wheel rotation unavailable: %s"), Error.message().c_str());
            QueueEvent(MoveTemp(Event));
        }
        catch (const std::exception& Error)
        {
            FArriettyBluetoothEvent Event;
            Event.Generation = WorkerGeneration;
            Event.Type = EArriettyBluetoothEventType::CscUnavailable;
            Event.Message = UTF8_TO_TCHAR(Error.what());
            QueueEvent(MoveTemp(Event));
        }

        BluetoothLEDevice HeartRateDevice{nullptr};
        GattDeviceService HeartRateService{nullptr};
        GattCharacteristic HeartRateMeasurement{nullptr};
        event_token HeartRateToken{};
        bool bHeartRateHandlerRegistered = false;
        bool bHeartRateEnabled = false;
        bool bHeartRateDisconnectReported = false;

        const auto CloseHeartRate = [&]
        {
            if (bHeartRateHandlerRegistered && HeartRateMeasurement)
            {
                try
                {
                    HeartRateMeasurement.ValueChanged(HeartRateToken);
                }
                catch (...)
                {
                }
            }
            bHeartRateHandlerRegistered = false;
            bHeartRateEnabled = false;
            if (HeartRateService) HeartRateService.Close();
            if (HeartRateDevice) HeartRateDevice.Close();
            HeartRateMeasurement = nullptr;
            HeartRateService = nullptr;
            HeartRateDevice = nullptr;
        };

        const auto TryConnectHeartRate = [&](uint64 Address)
        {
            CloseHeartRate();
            try
            {
                HeartRateDevice = BluetoothLEDevice::FromBluetoothAddressAsync(
                    Address).get();
                if (!HeartRateDevice)
                {
                    throw std::runtime_error("Windows could not open the BLE heart-rate sensor");
                }
                const auto ServiceResult = HeartRateDevice.GetGattServicesForUuidAsync(
                    HeartRateServiceUuid, BluetoothCacheMode::Uncached).get();
                if (ServiceResult.Status() != GattCommunicationStatus::Success ||
                    ServiceResult.Services().Size() == 0)
                {
                    throw std::runtime_error("Heart Rate service 0x180D is unavailable");
                }
                HeartRateService = ServiceResult.Services().GetAt(0);
                const auto CharacteristicResult = HeartRateService.GetCharacteristicsForUuidAsync(
                    HeartRateMeasurementUuid, BluetoothCacheMode::Uncached).get();
                if (CharacteristicResult.Status() != GattCommunicationStatus::Success ||
                    CharacteristicResult.Characteristics().Size() == 0)
                {
                    throw std::runtime_error("Heart Rate Measurement 0x2A37 is unavailable");
                }
                HeartRateMeasurement = CharacteristicResult.Characteristics().GetAt(0);
                HeartRateToken = HeartRateMeasurement.ValueChanged(
                    [this, WorkerGeneration](GattCharacteristic const&, GattValueChangedEventArgs const& Args)
                    {
                        winrt::com_array<uint8_t> Bytes;
                        CryptographicBuffer::CopyToByteArray(Args.CharacteristicValue(), Bytes);
                        const TOptional<uint16> HeartRate =
                            ArriettyTrainerProtocol::ParseHeartRateMeasurement(
                                MakeArrayView(Bytes.data(), static_cast<int32>(Bytes.size())));
                        if (!HeartRate.IsSet())
                        {
                            return;
                        }
                        FArriettyBluetoothEvent Event;
                        Event.Generation = WorkerGeneration;
                        Event.Type = EArriettyBluetoothEventType::HeartRateSample;
                        Event.HeartRateBpm = HeartRate.GetValue();
                        Event.ReceivedAtSeconds = FPlatformTime::Seconds();
                        QueueEvent(MoveTemp(Event));
                    });
                bHeartRateHandlerRegistered = true;
                if (HeartRateMeasurement.WriteClientCharacteristicConfigurationDescriptorAsync(
                        GattClientCharacteristicConfigurationDescriptorValue::Notify).get()
                    != GattCommunicationStatus::Success)
                {
                    throw std::runtime_error("Could not subscribe to Heart Rate Measurement 0x2A37");
                }
                bHeartRateEnabled = true;
                bHeartRateDisconnectReported = false;

                FArriettyBluetoothEvent Event;
                Event.Generation = WorkerGeneration;
                Event.Type = EArriettyBluetoothEventType::HeartRateConnected;
                Event.Message = FString::Printf(
                    TEXT("CONNECTED: %s"), HeartRateDevice.Name().c_str());
                QueueEvent(MoveTemp(Event));
                return true;
            }
            catch (const winrt::hresult_error& Error)
            {
                CloseHeartRate();
                FArriettyBluetoothEvent Event;
                Event.Generation = WorkerGeneration;
                Event.Type = EArriettyBluetoothEventType::HeartRateUnavailable;
                Event.Message = FString::Printf(
                    TEXT("UNAVAILABLE: Windows Bluetooth 0x%08x"),
                    static_cast<uint32>(Error.code().value));
                QueueEvent(MoveTemp(Event));
                return false;
            }
            catch (const std::exception& Error)
            {
                CloseHeartRate();
                FArriettyBluetoothEvent Event;
                Event.Generation = WorkerGeneration;
                Event.Type = EArriettyBluetoothEventType::HeartRateUnavailable;
                Event.Message = FString::Printf(TEXT("UNAVAILABLE: %s"), UTF8_TO_TCHAR(Error.what()));
                QueueEvent(MoveTemp(Event));
                return false;
            }
        };

        if (HeartRateDeviceAddress != 0)
        {
            TryConnectHeartRate(HeartRateDeviceAddress);
        }

        std::atomic<uint64> PendingHeartRateAddress{0};
        BluetoothLEAdvertisementWatcher HeartRateWatcher;
        HeartRateWatcher.ScanningMode(BluetoothLEScanningMode::Active);
        const event_token HeartRateScanToken = HeartRateWatcher.Received(
            [&](BluetoothLEAdvertisementWatcher const&,
                BluetoothLEAdvertisementReceivedEventArgs const& Args)
            {
                for (const winrt::guid& ServiceUuid : Args.Advertisement().ServiceUuids())
                {
                    if (ServiceUuid == HeartRateServiceUuid)
                    {
                        PendingHeartRateAddress.store(Args.BluetoothAddress());
                        break;
                    }
                }
            });
        bool bHeartRateWatcherRunning = false;
        const auto StartHeartRateWatcher = [&]
        {
            if (!bHeartRateWatcherRunning)
            {
                try
                {
                    PendingHeartRateAddress.store(0);
                    HeartRateWatcher.Start();
                    bHeartRateWatcherRunning = true;
                }
                catch (const winrt::hresult_error& Error)
                {
                    FArriettyBluetoothEvent Event;
                    Event.Generation = WorkerGeneration;
                    Event.Type = EArriettyBluetoothEventType::HeartRateUnavailable;
                    Event.Message = FString::Printf(
                        TEXT("SCAN UNAVAILABLE: Windows Bluetooth 0x%08x"),
                        static_cast<uint32>(Error.code().value));
                    QueueEvent(MoveTemp(Event));
                }
            }
        };
        const auto StopHeartRateWatcher = [&]
        {
            if (bHeartRateWatcherRunning)
            {
                try
                {
                    HeartRateWatcher.Stop();
                }
                catch (...)
                {
                }
                bHeartRateWatcherRunning = false;
            }
        };
        if (!bHeartRateEnabled)
        {
            StartHeartRateWatcher();
            FArriettyBluetoothEvent Event;
            Event.Generation = WorkerGeneration;
            Event.Type = EArriettyBluetoothEventType::HeartRateUnavailable;
            Event.Message = TEXT("SEARCHING: enable Garmin Broadcast Heart Rate (BLE)");
            QueueEvent(MoveTemp(Event));
        }

        {
            FArriettyBluetoothEvent Event;
            Event.Generation = WorkerGeneration;
            Event.Type = EArriettyBluetoothEventType::Connected;
            QueueEvent(MoveTemp(Event));
        }

        double NextHeartRateConnectionAttemptSeconds = 0.0;
        while (!bStopRequested.Load() && Device.ConnectionStatus() == BluetoothConnectionStatus::Connected)
        {
            FControlRequest Request;
            bool bControlChanged = false;
            while (ControlRequests.Dequeue(Request))
            {
                if (Request.Generation == WorkerGeneration)
                {
                    if (Request.PresetIndex.IsSet())
                    {
                        CurrentPresetIndex = Request.PresetIndex.GetValue();
                        bControlChanged = true;
                    }
                    if (Request.GradePercent.IsSet())
                    {
                        CurrentGradePercent = Request.GradePercent.GetValue();
                        bControlChanged = true;
                    }
                }
            }
            if (bControlChanged)
            {
                SendControlCommand(ArriettyTrainerProtocol::BuildSimulationControlCommand(
                    CurrentPresetIndex, CurrentGradePercent));
                FArriettyBluetoothEvent Event;
                Event.Generation = WorkerGeneration;
                Event.Type = EArriettyBluetoothEventType::ControlReady;
                Event.PresetIndex = CurrentPresetIndex;
                Event.GradePercent = CurrentGradePercent;
                QueueEvent(MoveTemp(Event));
            }
            const double NowSeconds = FPlatformTime::Seconds();
            if (!bHeartRateEnabled && NowSeconds >= NextHeartRateConnectionAttemptSeconds)
            {
                const uint64 PendingAddress = PendingHeartRateAddress.exchange(0);
                if (PendingAddress != 0)
                {
                    if (TryConnectHeartRate(PendingAddress))
                    {
                        StopHeartRateWatcher();
                    }
                    else
                    {
                        NextHeartRateConnectionAttemptSeconds = NowSeconds + 5.0;
                    }
                }
            }
            if (bHeartRateEnabled && !bHeartRateDisconnectReported &&
                HeartRateDevice.ConnectionStatus() != BluetoothConnectionStatus::Connected)
            {
                bHeartRateDisconnectReported = true;
                FArriettyBluetoothEvent Event;
                Event.Generation = WorkerGeneration;
                Event.Type = EArriettyBluetoothEventType::HeartRateUnavailable;
                Event.Message = TEXT("DISCONNECTED - SEARCHING");
                QueueEvent(MoveTemp(Event));
                CloseHeartRate();
                StartHeartRateWatcher();
                NextHeartRateConnectionAttemptSeconds = NowSeconds + 1.0;
            }
            FPlatformProcess::Sleep(0.1f);
        }

        const bool bTrainerConnectionLost = !bStopRequested.Load();

        if (bCscEnabled)
        {
            CscMeasurement.WriteClientCharacteristicConfigurationDescriptorAsync(
                GattClientCharacteristicConfigurationDescriptorValue::None).get();
            CscMeasurement.ValueChanged(CscToken);
        }
        StopHeartRateWatcher();
        HeartRateWatcher.Received(HeartRateScanToken);
        if (bHeartRateEnabled)
        {
            try
            {
                HeartRateMeasurement.WriteClientCharacteristicConfigurationDescriptorAsync(
                    GattClientCharacteristicConfigurationDescriptorValue::None).get();
            }
            catch (...)
            {
            }
        }
        CloseHeartRate();
        IndoorBikeData.WriteClientCharacteristicConfigurationDescriptorAsync(
            GattClientCharacteristicConfigurationDescriptorValue::None).get();
        IndoorBikeData.ValueChanged(TrainerToken);
        ControlPoint.WriteClientCharacteristicConfigurationDescriptorAsync(
            GattClientCharacteristicConfigurationDescriptorValue::None).get();
        ControlPoint.ValueChanged(ControlToken);
        if (CscService) CscService.Close();
        FtmsService.Close();
        Device.Close();
        if (bTrainerConnectionLost)
        {
            throw std::runtime_error("The T2 Bluetooth connection was lost");
        }
    }
    catch (const winrt::hresult_error& Error)
    {
        if (!bStopRequested.Load())
        {
            QueueError(WorkerGeneration, FString::Printf(
                TEXT("Windows Bluetooth error 0x%08x: %s"),
                static_cast<uint32>(Error.code().value),
                Error.message().c_str()));
        }
    }
    catch (const std::exception& Error)
    {
        if (!bStopRequested.Load())
        {
            QueueError(WorkerGeneration, UTF8_TO_TCHAR(Error.what()));
        }
    }
    if (bApartmentInitialized)
    {
        winrt::uninit_apartment();
    }
#endif

    bWorkerRunning.Store(false);
    FArriettyBluetoothEvent Event;
    Event.Generation = WorkerGeneration;
    Event.Type = EArriettyBluetoothEventType::WorkerStopped;
    QueueEvent(MoveTemp(Event));
}
