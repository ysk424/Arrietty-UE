// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyFanController.h"

#include "ArriettyTypes.h"
#include "IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

DEFINE_LOG_CATEGORY_STATIC(LogArriettyFan, Log, All);

FArriettyFanController::~FArriettyFanController()
{
    Stop();
}

bool FArriettyFanController::Start()
{
    Stop();
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogArriettyFan, Error, TEXT("Socket subsystem is unavailable"));
        return false;
    }

    Socket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("ArriettyFanUdp"), false);
    if (!Socket)
    {
        UE_LOG(LogArriettyFan, Error, TEXT("Could not create fan UDP socket"));
        return false;
    }
    Socket->SetNonBlocking(true);
    Destination = SocketSubsystem->CreateInternetAddr();
    bool bValidAddress = false;
    Destination->SetIp(TEXT("192.168.4.1"), bValidAddress);
    Destination->SetPort(Arrietty::FanUdpPort);
    if (!bValidAddress)
    {
        UE_LOG(LogArriettyFan, Error, TEXT("Invalid ESP32 fan address"));
        Stop();
        return false;
    }

    LastRequestedLevel = INDEX_NONE;
    ReportedLevel = 0;
    LastSendSeconds = -1.0;
    UE_LOG(LogArriettyFan, Display, TEXT("Fan UDP target 192.168.4.1:%d"), Arrietty::FanUdpPort);
    return true;
}

void FArriettyFanController::Stop()
{
    if (!Socket)
    {
        Destination.Reset();
        return;
    }
    SendCommand(TEXT("LEVEL 0"));
    ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
    Socket = nullptr;
    Destination.Reset();
}

int32 FArriettyFanController::LevelForSpeed(double SpeedKmh)
{
    if (SpeedKmh <= Arrietty::FanStoppedThresholdKmh)
    {
        return 0;
    }
    const double Fraction = FMath::Clamp(SpeedKmh / Arrietty::FanMaximumSpeedKmh, 0.0, 1.0);
    return FMath::Clamp(
        FMath::CeilToInt(Fraction * Arrietty::FanLevelCount),
        1,
        Arrietty::FanLevelCount);
}

void FArriettyFanController::Tick(double SpeedKmh, double NowSeconds)
{
    if (!Socket || !Destination.IsValid())
    {
        return;
    }
    const int32 RequestedLevel = LevelForSpeed(SpeedKmh);
    if (RequestedLevel == LastRequestedLevel &&
        LastSendSeconds >= 0.0 &&
        NowSeconds - LastSendSeconds < Arrietty::FanResendSeconds)
    {
        return;
    }
    if (SendCommand(FString::Printf(TEXT("LEVEL %d"), RequestedLevel)))
    {
        LastRequestedLevel = RequestedLevel;
        ReportedLevel = RequestedLevel;
        LastSendSeconds = NowSeconds;
    }
}

void FArriettyFanController::CorrectReportedLevel(int32 Delta)
{
    ReportedLevel = FMath::Clamp(ReportedLevel + Delta, 0, Arrietty::FanLevelCount);
    SendCommand(FString::Printf(TEXT("SYNC %d"), ReportedLevel));
    LastRequestedLevel = INDEX_NONE;
    UE_LOG(LogArriettyFan, Display, TEXT("Fan state corrected to level %d"), ReportedLevel);
}

bool FArriettyFanController::SendCommand(const FString& Command)
{
    if (!Socket || !Destination.IsValid())
    {
        return false;
    }
    FTCHARToUTF8 Utf8(*Command);
    int32 BytesSent = 0;
    const bool bSent = Socket->SendTo(
        reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), BytesSent, *Destination);
    if (!bSent || BytesSent != Utf8.Length())
    {
        UE_LOG(LogArriettyFan, Warning, TEXT("Failed to send fan command: %s"), *Command);
        return false;
    }
    UE_LOG(LogArriettyFan, Verbose, TEXT("Fan command: %s"), *Command);
    return true;
}
