// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ArriettyNavigationComponent.h"
#include "ArriettyCesiumNavigationComponent.generated.h"

class ACesiumGeoreference;
class UCesiumEllipsoid;
class UCesiumGlobeAnchorComponent;

/** Maps Arrietty's meter-based simulation onto Cesium's WGS84 globe. */
UCLASS(ClassGroup = "Arrietty|Cesium", Meta = (BlueprintSpawnableComponent))
class ARRIETTYCESIUM_API UArriettyCesiumNavigationComponent
    : public UArriettyNavigationComponent
{
    GENERATED_BODY()

public:
    UArriettyCesiumNavigationComponent();

    virtual bool InitializeNavigation(
        const FVector2D& LocalPositionMeters,
        double HeightMeters) override;
    virtual bool ApplyNavigationPose(
        const FVector2D& LocalPositionMeters,
        double HeightMeters,
        double HeadingDegrees,
        double PitchDegrees,
        double BankDegrees) override;
    virtual bool BuildRideSurfaceTrace(
        const FVector2D& LocalPositionMeters,
        FVector& OutStart,
        FVector& OutEnd) const override;
    virtual double HeightMetersFromWorldLocation(const FVector& WorldLocation) const override;
    virtual bool GetGeospatialCoordinates(
        double& OutLongitudeDegrees,
        double& OutLatitudeDegrees,
        double& OutEllipsoidHeightMeters) const override;

private:
    bool ResolveCesiumObjects();
    bool PredictLongitudeLatitudeHeight(
        const FVector2D& LocalPositionMeters,
        FVector& OutLongitudeLatitudeHeight) const;

    UPROPERTY(Transient)
    TObjectPtr<UCesiumGlobeAnchorComponent> GlobeAnchor;

    UPROPERTY(Transient)
    TObjectPtr<ACesiumGeoreference> Georeference;

    UPROPERTY(Transient)
    TObjectPtr<UCesiumEllipsoid> Ellipsoid;

    FVector2D LastLocalPositionMeters = FVector2D::ZeroVector;
    double BaseEllipsoidHeightMeters = 0.0;
    bool bInitialized = false;
};
