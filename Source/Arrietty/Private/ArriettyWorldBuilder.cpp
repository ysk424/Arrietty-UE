// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ArriettyWorldBuilder.h"

#include "ArriettyTypes.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AArriettyWorldBuilder::AArriettyWorldBuilder()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("World Root"));
    SetRootComponent(SceneRoot);

    SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun Light"));
    SunLight->SetupAttachment(SceneRoot);
    SunLight->SetRelativeRotation(FRotator(-38.0, -35.0, 0.0));
    SunLight->SetIntensity(7.0f);
    SunLight->SetMobility(EComponentMobility::Movable);

    SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Sky Atmosphere"));
    SkyAtmosphere->SetupAttachment(SceneRoot);

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("Sky Light"));
    SkyLight->SetupAttachment(SceneRoot);
    SkyLight->SetIntensity(1.0f);
    SkyLight->SetMobility(EComponentMobility::Movable);
    SkyLight->SetRealTimeCapture(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    Terrain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lightweight Terrain"));
    Terrain->SetupAttachment(SceneRoot);
    Terrain->SetStaticMesh(PlaneMesh.Object);
    Terrain->SetRelativeLocation(FVector(0.0, 0.0, -25.0));
    Terrain->SetRelativeScale3D(FVector(3200.0, 2400.0, 1.0));
    Terrain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Terrain->SetCastShadow(false);

    Lake = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mirror Lake"));
    Lake->SetupAttachment(SceneRoot);
    Lake->SetStaticMesh(PlaneMesh.Object);
    Lake->SetRelativeLocation(FVector(0.0, 0.0, -5.0));
    Lake->SetRelativeScale3D(FVector(380.0, 220.0, 1.0));
    Lake->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Lake->SetCastShadow(false);

    RoadSegments = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Secret World Ride Surface"));
    RoadSegments->SetupAttachment(SceneRoot);
    RoadSegments->SetStaticMesh(CubeMesh.Object);
    RoadSegments->ComponentTags.Add(FName(Arrietty::RideSurfaceTag));
    RoadSegments->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    RoadSegments->SetCollisionResponseToAllChannels(ECR_Ignore);
    RoadSegments->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    RoadSegments->SetCanEverAffectNavigation(false);
    RoadSegments->SetCastShadow(false);

    Buildings = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Village Buildings"));
    Buildings->SetupAttachment(SceneRoot);
    Buildings->SetStaticMesh(CubeMesh.Object);
    Buildings->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Buildings->SetCollisionResponseToAllChannels(ECR_Block);
    Buildings->SetCanEverAffectNavigation(false);

    RingSegments = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Flight Rings"));
    RingSegments->SetupAttachment(SceneRoot);
    RingSegments->SetStaticMesh(CylinderMesh.Object);
    RingSegments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RingSegments->SetCanEverAffectNavigation(false);
    RingSegments->SetCastShadow(false);
}

void AArriettyWorldBuilder::BeginPlay()
{
    Super::BeginPlay();
    BuildWorld();
    const auto SetColor = [](UPrimitiveComponent* Component, const FLinearColor& Color)
    {
        if (Component)
        {
            if (UMaterialInstanceDynamic* Material = Component->CreateAndSetMaterialInstanceDynamic(0))
            {
                Material->SetVectorParameterValue(TEXT("Color"), Color);
            }
        }
    };
    SetColor(Terrain, FLinearColor(0.025f, 0.16f, 0.055f));
    SetColor(Lake, FLinearColor(0.01f, 0.15f, 0.38f));
    SetColor(RoadSegments, FLinearColor(0.055f, 0.065f, 0.075f));
    SetColor(Buildings, FLinearColor(0.30f, 0.34f, 0.38f));
    SetColor(RingSegments, FLinearColor(1.0f, 0.18f, 0.01f));
    SkyLight->RecaptureSky();
}

void AArriettyWorldBuilder::BuildWorld()
{
    if (RoadSegments->GetInstanceCount() > 0)
    {
        return;
    }
    BuildRoad();
    BuildBuildings();
    BuildFlightRings();
}

void AArriettyWorldBuilder::BuildRoad()
{
    constexpr int32 SegmentCount = 256;
    constexpr double RadiusXCentimeters = 50500.0;
    constexpr double RadiusYCentimeters = 32000.0;
    constexpr double RoadWidthCentimeters = 600.0;
    for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
    {
        const double T0 = 2.0 * UE_DOUBLE_PI * SegmentIndex / SegmentCount;
        const double T1 = 2.0 * UE_DOUBLE_PI * (SegmentIndex + 1) / SegmentCount;
        const FVector Start(
            RadiusXCentimeters * FMath::Sin(T0),
            RadiusYCentimeters * FMath::Cos(T0),
            0.0);
        const FVector End(
            RadiusXCentimeters * FMath::Sin(T1),
            RadiusYCentimeters * FMath::Cos(T1),
            0.0);
        const FVector Delta = End - Start;
        const FVector Midpoint = (Start + End) * 0.5;
        const double YawDegrees = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
        const FVector Scale(Delta.Size2D() / 100.0 + 0.15, RoadWidthCentimeters / 100.0, 0.12);
        RoadSegments->AddInstance(FTransform(FRotator(0.0, YawDegrees, 0.0), Midpoint, Scale));
    }
}

void AArriettyWorldBuilder::BuildBuildings()
{
    FRandomStream Random(424);
    for (int32 Index = 0; Index < 96; ++Index)
    {
        const double Angle = Random.FRandRange(0.0, 2.0 * UE_DOUBLE_PI);
        const double Radius = Random.FRandRange(7000.0, 21000.0);
        const double Width = Random.FRandRange(600.0, 1800.0);
        const double Depth = Random.FRandRange(600.0, 1800.0);
        const double Height = Random.FRandRange(800.0, 6500.0);
        const FVector Position(
            FMath::Cos(Angle) * Radius,
            FMath::Sin(Angle) * Radius,
            Height * 0.5);
        Buildings->AddInstance(FTransform(
            FRotator(0.0, Random.FRandRange(0.0, 360.0), 0.0),
            Position,
            FVector(Width / 100.0, Depth / 100.0, Height / 100.0)));
    }
}

void AArriettyWorldBuilder::BuildFlightRings()
{
    const TArray<FVector> RingCenters = {
        FVector(-18000.0, 0.0, 1800.0),
        FVector(-9000.0, -9000.0, 2800.0),
        FVector(0.0, -13000.0, 4200.0),
        FVector(10000.0, -7000.0, 3200.0),
        FVector(18000.0, 2000.0, 2200.0),
    };
    constexpr int32 RingSegmentCount = 24;
    constexpr double RingRadius = 600.0;
    for (const FVector& Center : RingCenters)
    {
        for (int32 Index = 0; Index < RingSegmentCount; ++Index)
        {
            const double A0 = 2.0 * UE_DOUBLE_PI * Index / RingSegmentCount;
            const double A1 = 2.0 * UE_DOUBLE_PI * (Index + 1) / RingSegmentCount;
            const FVector Start = Center + FVector(0.0, FMath::Cos(A0) * RingRadius, FMath::Sin(A0) * RingRadius);
            const FVector End = Center + FVector(0.0, FMath::Cos(A1) * RingRadius, FMath::Sin(A1) * RingRadius);
            const FVector Segment = End - Start;
            const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Segment.GetSafeNormal());
            RingSegments->AddInstance(FTransform(
                Rotation,
                (Start + End) * 0.5,
                FVector(0.30, 0.30, Segment.Size() / 100.0)));
        }
    }
}
