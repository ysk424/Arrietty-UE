// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArriettyWorldBuilder.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UDirectionalLightComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UStaticMeshComponent;

UCLASS()
class ARRIETTYRUNTIME_API AArriettyWorldBuilder : public AActor
{
    GENERATED_BODY()

public:
    AArriettyWorldBuilder();
    virtual void BeginPlay() override;

private:
    void BuildWorld();
    void BuildRoad();
    void BuildBuildings();
    void BuildFlightRings();

    UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> SunLight;
    UPROPERTY() TObjectPtr<USkyLightComponent> SkyLight;
    UPROPERTY() TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Terrain;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Lake;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoadSegments;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Buildings;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RingSegments;
};
