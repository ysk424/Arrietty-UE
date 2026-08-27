// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "ArriettyCourseStart.generated.h"

/** Place one in a Level to define the bicycle's initial position and heading. */
UCLASS(BlueprintType)
class ARRIETTYRUNTIME_API AArriettyCourseStart : public APlayerStart
{
    GENERATED_BODY()

public:
    AArriettyCourseStart(const FObjectInitializer& ObjectInitializer);
};
