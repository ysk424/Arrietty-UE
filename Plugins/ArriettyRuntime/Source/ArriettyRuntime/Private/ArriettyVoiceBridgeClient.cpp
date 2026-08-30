// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyVoiceBridgeClient.h"

#include "ArriettyTypes.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

FArriettyVoiceBridgeClient::FArriettyVoiceBridgeClient()
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        return;
    }
    Socket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("Arrietty Voice PTT"), false);
    Destination = SocketSubsystem->CreateInternetAddr();
    bool bValidAddress = false;
    Destination->SetIp(TEXT("127.0.0.1"), bValidAddress);
    Destination->SetPort(Arrietty::VoiceBridgePort);
    if (!Socket || !bValidAddress)
    {
        if (Socket)
        {
            SocketSubsystem->DestroySocket(Socket);
            Socket = nullptr;
        }
        Destination.Reset();
    }
    else
    {
        Socket->SetNonBlocking(true);
    }
}

FArriettyVoiceBridgeClient::~FArriettyVoiceBridgeClient()
{
    if (Socket)
    {
        if (ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
        {
            SocketSubsystem->DestroySocket(Socket);
        }
        Socket = nullptr;
    }
}

bool FArriettyVoiceBridgeClient::IsAvailable() const
{
    return Socket != nullptr && Destination.IsValid();
}

bool FArriettyVoiceBridgeClient::SendPttDown()
{
    return SendCommand("ARRIETTY_VOICE/1 PTT_DOWN");
}

bool FArriettyVoiceBridgeClient::SendPttUp()
{
    return SendCommand("ARRIETTY_VOICE/1 PTT_UP");
}

bool FArriettyVoiceBridgeClient::SendPttCancel()
{
    return SendCommand("ARRIETTY_VOICE/1 PTT_CANCEL");
}

bool FArriettyVoiceBridgeClient::PollStatus(FString& OutStatus, FString& OutDetail)
{
    if (!Socket)
    {
        return false;
    }

    bool bFoundStatus = false;
    uint32 PendingBytes = 0;
    while (Socket->HasPendingData(PendingBytes))
    {
        uint8 Buffer[2048]{};
        int32 BytesRead = 0;
        TSharedRef<FInternetAddr> Sender =
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        const int32 Capacity = FMath::Min<int32>(
            static_cast<int32>(PendingBytes),
            static_cast<int32>(sizeof(Buffer) - 1));
        if (!Socket->RecvFrom(Buffer, Capacity, BytesRead, *Sender) || BytesRead <= 0)
        {
            break;
        }
        Buffer[BytesRead] = 0;
        const FString Message = UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer));
        constexpr const TCHAR* Prefix = TEXT("ARRIETTY_VOICE/1 STATUS ");
        if (!Message.StartsWith(Prefix))
        {
            continue;
        }

        const FString Payload = Message.RightChop(FCString::Strlen(Prefix)).TrimStartAndEnd();
        FString Status;
        FString Detail;
        if (!Payload.Split(TEXT(" "), &Status, &Detail))
        {
            Status = Payload;
        }
        if (!Status.IsEmpty())
        {
            OutStatus = MoveTemp(Status);
            OutDetail = Detail.TrimStartAndEnd();
            bFoundStatus = true;
        }
    }
    return bFoundStatus;
}

bool FArriettyVoiceBridgeClient::SendCommand(const ANSICHAR* Command)
{
    if (!IsAvailable())
    {
        return false;
    }
    int32 BytesSent = 0;
    const int32 Length = FCStringAnsi::Strlen(Command);
    return Socket->SendTo(
        reinterpret_cast<const uint8*>(Command),
        Length,
        BytesSent,
        *Destination) && BytesSent == Length;
}
