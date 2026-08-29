// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArriettyNavigationComponent.generated.h"

/**
 * Optional navigation bridge for worlds whose coordinate system is not a
 * fixed Unreal tangent plane. The base implementation keeps the existing
 * local-world behaviour; integrations such as Cesium override these methods.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = "Arrietty")
class ARRIETTYRUNTIME_API UArriettyNavigationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UArriettyNavigationComponent();

    virtual bool InitializeNavigation(
        const FVector2D& LocalPositionMeters,
        double HeightMeters);
    virtual bool ApplyNavigationPose(
        const FVector2D& LocalPositionMeters,
        double HeightMeters,
        double HeadingDegrees,
        double PitchDegrees,
        double BankDegrees);
    virtual bool BuildRideSurfaceTrace(
        const FVector2D& LocalPositionMeters,
        FVector& OutStart,
        FVector& OutEnd) const;
    virtual double HeightMetersFromWorldLocation(const FVector& WorldLocation) const;
    virtual bool GetGeospatialCoordinates(
        double& OutLongitudeDegrees,
        double& OutLatitudeDegrees,
        double& OutEllipsoidHeightMeters) const;
};
