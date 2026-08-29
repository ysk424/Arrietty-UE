// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyPawn.h"
#include "ArriettyCesiumPawn.generated.h"

class UArriettyCesiumNavigationComponent;
class UCesiumGlobeAnchorComponent;
class UCesiumOriginShiftComponent;

UCLASS()
class ARRIETTYCESIUM_API AArriettyCesiumPawn : public AArriettyPawn
{
    GENERATED_BODY()

public:
    AArriettyCesiumPawn();

private:
    UPROPERTY(VisibleAnywhere, Category = "Cesium")
    TObjectPtr<UCesiumGlobeAnchorComponent> GlobeAnchor;

    UPROPERTY(VisibleAnywhere, Category = "Cesium")
    TObjectPtr<UCesiumOriginShiftComponent> OriginShift;

    UPROPERTY(VisibleAnywhere, Category = "Cesium")
    TObjectPtr<UArriettyCesiumNavigationComponent> CesiumNavigation;
};
