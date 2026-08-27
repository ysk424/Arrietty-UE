// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArriettyGameMode.generated.h"

class UArriettyControlWidget;

UCLASS()
class ARRIETTYRUNTIME_API AArriettyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AArriettyGameMode();
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual void StartPlay() override;

private:
    UPROPERTY()
    TObjectPtr<UArriettyControlWidget> ControlWidget;
};
