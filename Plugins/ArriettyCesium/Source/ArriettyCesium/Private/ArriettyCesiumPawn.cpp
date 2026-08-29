// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyCesiumPawn.h"

#include "ArriettyCesiumNavigationComponent.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumOriginShiftComponent.h"

AArriettyCesiumPawn::AArriettyCesiumPawn()
{
    GlobeAnchor = CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("Cesium Globe Anchor"));
    OriginShift = CreateDefaultSubobject<UCesiumOriginShiftComponent>(TEXT("Cesium Origin Shift"));
    OriginShift->SetMode(ECesiumOriginShiftMode::ChangeCesiumGeoreference);
    OriginShift->SetDistance(10000.0);
    CesiumNavigation = CreateDefaultSubobject<UArriettyCesiumNavigationComponent>(
        TEXT("Arrietty Cesium Navigation"));
}
