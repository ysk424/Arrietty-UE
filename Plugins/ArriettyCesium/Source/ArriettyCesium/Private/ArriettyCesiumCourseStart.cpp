// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyCesiumCourseStart.h"

#include "ArriettyTypes.h"
#include "CesiumGlobeAnchorComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AArriettyCesiumCourseStart::AArriettyCesiumCourseStart(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    GetRootComponent()->SetMobility(EComponentMobility::Movable);
    GlobeAnchor = CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("Cesium Globe Anchor"));

    Runway = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arrietty Landing Runway"));
    Runway->SetupAttachment(GetRootComponent());
    Runway->SetRelativeLocation(FVector(5000.0, 0.0, -5.0));
    Runway->SetRelativeScale3D(FVector(100.0, 12.0, 0.10));
    Runway->SetMobility(EComponentMobility::Movable);
    Runway->SetCollisionProfileName(TEXT("BlockAll"));
    Runway->ComponentTags.Add(FName(Arrietty::RideSurfaceTag));

    // Keep the visible runway compact, but provide enough invisible surface
    // for a human-powered aircraft to reach takeoff speed before the logical
    // course edge. The top of this box remains level with the runway surface.
    FlightRolloutSurface = CreateDefaultSubobject<UBoxComponent>(
        TEXT("Arrietty Flight Rollout Surface"));
    FlightRolloutSurface->SetupAttachment(GetRootComponent());
    FlightRolloutSurface->SetRelativeLocation(FVector(100000.0, 0.0, -5.0));
    FlightRolloutSurface->SetBoxExtent(FVector(100000.0, 600.0, 5.0));
    FlightRolloutSurface->SetMobility(EComponentMobility::Movable);
    FlightRolloutSurface->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FlightRolloutSurface->SetCollisionResponseToAllChannels(ECR_Ignore);
    FlightRolloutSurface->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    FlightRolloutSurface->SetHiddenInGame(true);
    FlightRolloutSurface->ComponentTags.Add(FName(Arrietty::RideSurfaceTag));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Runway->SetStaticMesh(CubeMesh.Object);
    }
}
