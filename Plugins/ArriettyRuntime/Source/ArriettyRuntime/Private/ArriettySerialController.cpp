// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettySerialController.h"

#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"

#include <string>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogArriettyController, Log, All);

namespace
{
#if PLATFORM_WINDOWS
bool ConfigureSerialPort(HANDLE Handle)
{
    DCB State{};
    State.DCBlength = sizeof(State);
    if (!GetCommState(Handle, &State))
    {
        return false;
    }
    State.BaudRate = CBR_115200;
    State.ByteSize = 8;
    State.Parity = NOPARITY;
    State.StopBits = ONESTOPBIT;
    State.fBinary = 1;
    State.fParity = 0;
    State.fDtrControl = DTR_CONTROL_DISABLE;
    State.fRtsControl = RTS_CONTROL_DISABLE;
    if (!SetCommState(Handle, &State))
    {
        return false;
    }

    COMMTIMEOUTS Timeouts{};
    Timeouts.ReadIntervalTimeout = 20;
    Timeouts.ReadTotalTimeoutConstant = 50;
    Timeouts.WriteTotalTimeoutConstant = 500;
    return SetCommTimeouts(Handle, &Timeouts) != 0;
}

bool SendLine(HANDLE Handle, const char* Line)
{
    const DWORD Length = static_cast<DWORD>(strlen(Line));
    DWORD Written = 0;
    return WriteFile(Handle, Line, Length, &Written, nullptr) != 0 && Written == Length;
}

bool ReadLine(
    HANDLE Handle,
    FString& OutLine,
    double TimeoutSeconds,
    const TAtomic<bool>& StopRequested)
{
    std::string Buffer;
    const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
    while (!StopRequested.Load() && FPlatformTime::Seconds() < Deadline)
    {
        char Character = 0;
        DWORD BytesRead = 0;
        if (!ReadFile(Handle, &Character, 1, &BytesRead, nullptr))
        {
            return false;
        }
        if (BytesRead == 0)
        {
            continue;
        }
        if (Character == '\n')
        {
            while (!Buffer.empty() && Buffer.back() == '\r')
            {
                Buffer.pop_back();
            }
            OutLine = UTF8_TO_TCHAR(Buffer.c_str());
            return true;
        }
        if (Buffer.size() < 255)
        {
            Buffer.push_back(Character);
        }
        else
        {
            Buffer.clear();
        }
    }
    return false;
}

bool WaitForIdentification(HANDLE Handle, const TAtomic<bool>& StopRequested)
{
    PurgeComm(Handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    const double ResetDeadline = FPlatformTime::Seconds() + 1.2;
    while (!StopRequested.Load() && FPlatformTime::Seconds() < ResetDeadline)
    {
        FPlatformProcess::SleepNoStats(0.02f);
    }
    PurgeComm(Handle, PURGE_RXCLEAR);
    if (StopRequested.Load() || !SendLine(Handle, "PING\n"))
    {
        return false;
    }

    const double Deadline = FPlatformTime::Seconds() + 1.5;
    while (!StopRequested.Load() && FPlatformTime::Seconds() < Deadline)
    {
        FString Line;
        if (ReadLine(Handle, Line, 0.25, StopRequested) &&
            Line == TEXT("PONG ARRIETTY-CONTROLLER/1"))
        {
            return true;
        }
    }
    return false;
}
#endif
}

FArriettySerialController::~FArriettySerialController()
{
    StopAndWait();
}

void FArriettySerialController::Start()
{
    StopAndWait();
    FArriettyControllerEvent Discarded;
    while (Events.Dequeue(Discarded))
    {
    }
    bStopRequested.Store(false);
    WorkerFuture = Async(EAsyncExecution::Thread, [this]
    {
        WorkerMain();
    });
}

void FArriettySerialController::RequestStop()
{
    bStopRequested.Store(true);
}

void FArriettySerialController::StopAndWait()
{
    RequestStop();
    if (WorkerFuture.IsValid())
    {
        WorkerFuture.Wait();
        WorkerFuture = TFuture<void>();
    }
}

bool FArriettySerialController::DequeueEvent(FArriettyControllerEvent& OutEvent)
{
    return Events.Dequeue(OutEvent);
}

void FArriettySerialController::QueueEvent(FArriettyControllerEvent&& Event)
{
    Events.Enqueue(MoveTemp(Event));
}

void FArriettySerialController::QueueStatus(const FString& Message)
{
    FArriettyControllerEvent Event;
    Event.Type = EArriettyControllerEventType::Status;
    Event.Message = Message;
    QueueEvent(MoveTemp(Event));
}

void FArriettySerialController::WorkerMain()
{
#if !PLATFORM_WINDOWS
    QueueStatus(TEXT("UNAVAILABLE: ESP32 controller currently requires Windows"));
#else
    QueueStatus(TEXT("SEARCHING: USB-SERIAL CH340 controller"));
    while (!bStopRequested.Load())
    {
        bool bConnectedThisPass = false;
        // Search high to low so the tested COM7 is attempted before legacy
        // COM1 and Bluetooth virtual serial ports.
        for (int32 PortNumber = 64; PortNumber >= 1 && !bStopRequested.Load(); --PortNumber)
        {
            const FString PortName = FString::Printf(TEXT("COM%d"), PortNumber);
            const FString DevicePath = FString::Printf(TEXT("\\\\.\\COM%d"), PortNumber);
            HANDLE Handle = CreateFileW(
                *DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
            if (Handle == INVALID_HANDLE_VALUE)
            {
                continue;
            }
            SetupComm(Handle, 4096, 4096);
            if (!ConfigureSerialPort(Handle) || !WaitForIdentification(Handle, bStopRequested))
            {
                CloseHandle(Handle);
                continue;
            }

            bConnectedThisPass = true;
            FArriettyControllerEvent Connected;
            Connected.Type = EArriettyControllerEventType::Connected;
            Connected.Message = FString::Printf(TEXT("CONNECTED: %s at 115200 bps"), *PortName);
            QueueEvent(MoveTemp(Connected));
            UE_LOG(LogArriettyController, Log, TEXT("Arrietty controller connected on %s"), *PortName);

            SendLine(Handle, "STREAM ON\n");
            double LastSampleAtSeconds = FPlatformTime::Seconds();
            while (!bStopRequested.Load())
            {
                FString Line;
                if (ReadLine(Handle, Line, 0.5, bStopRequested))
                {
                    FArriettyControllerSample Sample;
                    if (ArriettyControllerProtocol::ParseStateLine(Line, Sample))
                    {
                        Sample.ReceivedAtSeconds = FPlatformTime::Seconds();
                        LastSampleAtSeconds = Sample.ReceivedAtSeconds;
                        FArriettyControllerEvent SampleEvent;
                        SampleEvent.Type = EArriettyControllerEventType::Sample;
                        SampleEvent.Sample = Sample;
                        QueueEvent(MoveTemp(SampleEvent));
                    }
                }
                if (FPlatformTime::Seconds() - LastSampleAtSeconds > 2.0)
                {
                    break;
                }
            }

            SendLine(Handle, "STREAM OFF\n");
            CloseHandle(Handle);
            if (!bStopRequested.Load())
            {
                FArriettyControllerEvent Disconnected;
                Disconnected.Type = EArriettyControllerEventType::Disconnected;
                Disconnected.Message = FString::Printf(TEXT("DISCONNECTED: %s; reconnecting"), *PortName);
                QueueEvent(MoveTemp(Disconnected));
            }
            break;
        }

        if (!bStopRequested.Load())
        {
            if (!bConnectedThisPass)
            {
                QueueStatus(TEXT("SEARCHING: connect the ESP32 USB cable"));
            }
            const double RetryAt = FPlatformTime::Seconds() + 1.0;
            while (!bStopRequested.Load() && FPlatformTime::Seconds() < RetryAt)
            {
                FPlatformProcess::SleepNoStats(0.05f);
            }
        }
    }
#endif
}
