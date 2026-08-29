// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyCourseStart.h"
#include "ArriettyCesiumCourseStart.generated.h"

class UCesiumGlobeAnchorComponent;
class UBoxComponent;
class UStaticMeshComponent;

/** Globe-anchored start with a visible 100 m runway and an invisible 2 km rollout surface. */
UCLASS(BlueprintType)
class ARRIETTYCESIUM_API AArriettyCesiumCourseStart : public AArriettyCourseStart
{
    GENERATED_BODY()

public:
    AArriettyCesiumCourseStart(const FObjectInitializer& ObjectInitializer);

private:
    UPROPERTY(VisibleAnywhere, Category = "Cesium")
    TObjectPtr<UCesiumGlobeAnchorComponent> GlobeAnchor;

    UPROPERTY(VisibleAnywhere, Category = "Arrietty")
    TObjectPtr<UStaticMeshComponent> Runway;

    UPROPERTY(VisibleAnywhere, Category = "Arrietty")
    TObjectPtr<UBoxComponent> FlightRolloutSurface;
};
