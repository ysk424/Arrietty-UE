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
    TSharedRef<FInternetAddr> LocalAddress = SocketSubsystem->CreateInternetAddr();
    LocalAddress->SetAnyAddress();
    LocalAddress->SetPort(0);
    if (!Socket->Bind(*LocalAddress))
    {
        UE_LOG(LogArriettyFan, Error, TEXT("Could not bind fan UDP response socket"));
        Stop();
        return false;
    }
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
    RequestedLevel = 0;
    ReportedLevel = INDEX_NONE;
    LastSendSeconds = -1.0;
    LastResponseSeconds = -1.0;
    FirstUnansweredSendSeconds = -1.0;
    Status = TEXT("WAITING FOR ESP32");
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
    Status = TEXT("STOPPED");
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
    PollResponses(NowSeconds);
    RequestedLevel = LevelForSpeed(SpeedKmh);
    if (RequestedLevel == LastRequestedLevel &&
        LastSendSeconds >= 0.0 &&
        NowSeconds - LastSendSeconds < Arrietty::FanResendSeconds)
    {
        return;
    }
    if (SendCommand(FString::Printf(TEXT("LEVEL %d"), RequestedLevel)))
    {
        LastRequestedLevel = RequestedLevel;
        LastSendSeconds = NowSeconds;
        if (FirstUnansweredSendSeconds < 0.0)
        {
            FirstUnansweredSendSeconds = NowSeconds;
        }
        if (LastResponseSeconds < 0.0)
        {
            Status = TEXT("WAITING FOR ESP32");
        }
    }
    PollResponses(NowSeconds);
}

void FArriettyFanController::CorrectReportedLevel(int32 Delta)
{
    const int32 BaseLevel = ReportedLevel == INDEX_NONE ? RequestedLevel : ReportedLevel;
    const int32 CorrectedLevel = FMath::Clamp(
        BaseLevel + Delta, 0, Arrietty::FanLevelCount);
    SendCommand(FString::Printf(TEXT("SYNC %d"), CorrectedLevel));
    RequestedLevel = CorrectedLevel;
    ReportedLevel = CorrectedLevel;
    LastRequestedLevel = INDEX_NONE;
    Status = TEXT("WAITING FOR SYNC ACK");
    UE_LOG(LogArriettyFan, Display, TEXT("Fan state correction requested: level %d"), CorrectedLevel);
}

TOptional<int32> FArriettyFanController::ParseResponseLevel(const FString& Response)
{
    FString Trimmed = Response;
    Trimmed.TrimStartAndEndInline();
    static const FString LevelPrefix = TEXT("OK LEVEL ");
    static const FString SyncPrefix = TEXT("OK SYNC ");
    const FString* Prefix = Trimmed.StartsWith(LevelPrefix, ESearchCase::IgnoreCase)
        ? &LevelPrefix
        : (Trimmed.StartsWith(SyncPrefix, ESearchCase::IgnoreCase) ? &SyncPrefix : nullptr);
    if (!Prefix)
    {
        return {};
    }
    FString LevelText = Trimmed.Mid(Prefix->Len());
    FString Unused;
    FString NumericText;
    if (LevelText.Split(TEXT(" "), &NumericText, &Unused))
    {
        LevelText = MoveTemp(NumericText);
    }
    if (!LevelText.IsNumeric())
    {
        return {};
    }
    const int32 Level = FCString::Atoi(*LevelText);
    if (Level < 0 || Level > Arrietty::FanLevelCount)
    {
        return {};
    }
    return Level;
}

void FArriettyFanController::PollResponses(double NowSeconds)
{
    if (!Socket)
    {
        return;
    }
    uint32 PendingSize = 0;
    while (Socket->HasPendingData(PendingSize))
    {
        TArray<uint8> Buffer;
        Buffer.SetNumUninitialized(FMath::Min<uint32>(PendingSize, 1024));
        TSharedRef<FInternetAddr> Sender =
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        int32 BytesRead = 0;
        if (!Socket->RecvFrom(Buffer.GetData(), Buffer.Num(), BytesRead, *Sender) || BytesRead <= 0)
        {
            break;
        }
        const FUTF8ToTCHAR Converted(
            reinterpret_cast<const ANSICHAR*>(Buffer.GetData()), BytesRead);
        const FString Response(Converted.Length(), Converted.Get());
        const TOptional<int32> Level = ParseResponseLevel(Response);
        if (!Level.IsSet())
        {
            UE_LOG(LogArriettyFan, Warning, TEXT("Unexpected ESP32 fan response: %s"), *Response);
            continue;
        }
        ReportedLevel = Level.GetValue();
        LastResponseSeconds = NowSeconds;
        FirstUnansweredSendSeconds = -1.0;
        Status = FString::Printf(TEXT("CONNECTED LEVEL %d"), ReportedLevel);
        UE_LOG(LogArriettyFan, Display, TEXT("Fan response: %s"), *Response);
    }
    if (FirstUnansweredSendSeconds >= 0.0 &&
        NowSeconds - FirstUnansweredSendSeconds >= 12.0)
    {
        Status = TEXT("NO RESPONSE - CONNECT WI-FI Arrietty-Fan");
    }
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
