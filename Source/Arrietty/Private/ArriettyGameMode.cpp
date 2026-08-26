// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ArriettyGameMode.h"

#include "ArriettyControlWidget.h"
#include "ArriettyPawn.h"
#include "ArriettyWorldBuilder.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AArriettyGameMode::AArriettyGameMode()
{
    DefaultPawnClass = AArriettyPawn::StaticClass();
}

void AArriettyGameMode::StartPlay()
{
    if (!UGameplayStatics::GetActorOfClass(this, AArriettyWorldBuilder::StaticClass()))
    {
        GetWorld()->SpawnActor<AArriettyWorldBuilder>(FVector::ZeroVector, FRotator::ZeroRotator);
    }
    Super::StartPlay();

    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        ControlWidget = CreateWidget<UArriettyControlWidget>(
            PlayerController,
            UArriettyControlWidget::StaticClass());
        if (ControlWidget)
        {
            ControlWidget->AddToViewport(100);
        }
        PlayerController->bShowMouseCursor = true;
        PlayerController->SetInputMode(FInputModeGameAndUI());
    }
}
