// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyGameMode.h"

#include "ArriettyCourseStart.h"
#include "ArriettyControlWidget.h"
#include "ArriettyPawn.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AArriettyGameMode::AArriettyGameMode()
{
    DefaultPawnClass = AArriettyPawn::StaticClass();
}

AActor* AArriettyGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    for (TActorIterator<AArriettyCourseStart> It(GetWorld()); It; ++It)
    {
        return *It;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

void AArriettyGameMode::StartPlay()
{
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
