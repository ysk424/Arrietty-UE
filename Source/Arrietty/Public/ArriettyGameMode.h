// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArriettyGameMode.generated.h"

class UArriettyControlWidget;

UCLASS()
class ARRIETTY_API AArriettyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AArriettyGameMode();
    virtual void StartPlay() override;

private:
    UPROPERTY()
    TObjectPtr<UArriettyControlWidget> ControlWidget;
};
