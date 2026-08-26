// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyBluetoothManager.h"

#include "ArriettyTrainerProtocol.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"

#include <chrono>
#include <algorithm>
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

void FArriettyBluetoothManager::Start(int32 InitialPresetIndex)
{
    StopAndWait();
    FArriettyBluetoothEvent Discarded;
    while (Events.Dequeue(Discarded))
    {
    }
    TPair<int32, int32> DiscardedRequest;
    while (PresetRequests.Dequeue(DiscardedRequest))
    {
    }

    const int32 WorkerGeneration = Generation.Load() + 1;
    Generation.Store(WorkerGeneration);
    bStopRequested.Store(false);
    bWorkerRunning.Store(true);
    WorkerFuture = Async(EAsyncExecution::Thread, [this, WorkerGeneration, InitialPresetIndex]
    {
        WorkerMain(WorkerGeneration, InitialPresetIndex);
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
    PresetRequests.Enqueue(TPair<int32, int32>(Generation.Load(), PresetIndex));
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

void FArriettyBluetoothManager::WorkerMain(int32 WorkerGeneration, int32 InitialPresetIndex)
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

    bool bApartmentInitialized = false;
    try
    {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        bApartmentInitialized = true;
        QueueStatus(WorkerGeneration, EArriettyRideStatus::Searching, TEXT("Searching for CYCPLUS T2"));

        std::mutex ScanMutex;
        std::condition_variable ScanCondition;
        uint64 DeviceAddress = 0;
        BluetoothLEAdvertisementWatcher Watcher;
        Watcher.ScanningMode(BluetoothLEScanningMode::Active);
        const event_token ScanToken = Watcher.Received(
            [&](BluetoothLEAdvertisementWatcher const&, BluetoothLEAdvertisementReceivedEventArgs const& Args)
            {
                const std::wstring Name(Args.Advertisement().LocalName().c_str());
                std::wstring LowerName = Name;
                std::transform(LowerName.begin(), LowerName.end(), LowerName.begin(), ::towlower);
                if (LowerName.find(L"t2") == std::wstring::npos)
                {
                    return;
                }
                {
                    std::lock_guard Lock(ScanMutex);
                    DeviceAddress = Args.BluetoothAddress();
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
        SendControlCommand(ArriettyTrainerProtocol::BuildFlatRoadControlCommand(InitialPresetIndex));
        {
            FArriettyBluetoothEvent Event;
            Event.Generation = WorkerGeneration;
            Event.Type = EArriettyBluetoothEventType::ControlReady;
            Event.PresetIndex = InitialPresetIndex;
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

        {
            FArriettyBluetoothEvent Event;
            Event.Generation = WorkerGeneration;
            Event.Type = EArriettyBluetoothEventType::Connected;
            QueueEvent(MoveTemp(Event));
        }

        while (!bStopRequested.Load() && Device.ConnectionStatus() == BluetoothConnectionStatus::Connected)
        {
            TPair<int32, int32> Request;
            TOptional<int32> LatestPreset;
            while (PresetRequests.Dequeue(Request))
            {
                if (Request.Key == WorkerGeneration)
                {
                    LatestPreset = Request.Value;
                }
            }
            if (LatestPreset.IsSet())
            {
                SendControlCommand(ArriettyTrainerProtocol::BuildFlatRoadControlCommand(LatestPreset.GetValue()));
                FArriettyBluetoothEvent Event;
                Event.Generation = WorkerGeneration;
                Event.Type = EArriettyBluetoothEventType::ControlReady;
                Event.PresetIndex = LatestPreset.GetValue();
                QueueEvent(MoveTemp(Event));
            }
            FPlatformProcess::Sleep(0.1f);
        }

        if (!bStopRequested.Load())
        {
            throw std::runtime_error("The T2 Bluetooth connection was lost");
        }

        if (bCscEnabled)
        {
            CscMeasurement.WriteClientCharacteristicConfigurationDescriptorAsync(
                GattClientCharacteristicConfigurationDescriptorValue::None).get();
            CscMeasurement.ValueChanged(CscToken);
        }
        IndoorBikeData.WriteClientCharacteristicConfigurationDescriptorAsync(
            GattClientCharacteristicConfigurationDescriptorValue::None).get();
        IndoorBikeData.ValueChanged(TrainerToken);
        ControlPoint.WriteClientCharacteristicConfigurationDescriptorAsync(
            GattClientCharacteristicConfigurationDescriptorValue::None).get();
        ControlPoint.ValueChanged(ControlToken);
        if (CscService) CscService.Close();
        FtmsService.Close();
        Device.Close();
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
