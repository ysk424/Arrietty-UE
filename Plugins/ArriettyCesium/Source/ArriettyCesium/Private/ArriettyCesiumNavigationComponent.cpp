// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyCesiumNavigationComponent.h"

#include "CesiumEllipsoid.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "GameFramework/Actor.h"

UArriettyCesiumNavigationComponent::UArriettyCesiumNavigationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UArriettyCesiumNavigationComponent::ResolveCesiumObjects()
{
    GlobeAnchor = GetOwner()
        ? GetOwner()->FindComponentByClass<UCesiumGlobeAnchorComponent>()
        : nullptr;
    if (!GlobeAnchor)
    {
        return false;
    }
    Georeference = GlobeAnchor->ResolveGeoreference();
    Ellipsoid = GlobeAnchor->GetEllipsoid();
    return Georeference != nullptr && Ellipsoid != nullptr;
}

bool UArriettyCesiumNavigationComponent::InitializeNavigation(
    const FVector2D& LocalPositionMeters,
    double HeightMeters)
{
    if (!ResolveCesiumObjects())
    {
        return false;
    }
    const FVector LongitudeLatitudeHeight = GlobeAnchor->GetLongitudeLatitudeHeight();
    BaseEllipsoidHeightMeters = LongitudeLatitudeHeight.Z - HeightMeters;
    LastLocalPositionMeters = LocalPositionMeters;
    bInitialized = true;
    return true;
}

bool UArriettyCesiumNavigationComponent::PredictLongitudeLatitudeHeight(
    const FVector2D& LocalPositionMeters,
    FVector& OutLongitudeLatitudeHeight) const
{
    if (!bInitialized || !GlobeAnchor || !Ellipsoid)
    {
        return false;
    }

    const FVector CurrentEcef = GlobeAnchor->GetEarthCenteredEarthFixedPosition();
    const FVector2D DeltaMeters = LocalPositionMeters - LastLocalPositionMeters;
    const FMatrix EastNorthUpToEcef =
        Ellipsoid->EastNorthUpToEllipsoidCenteredEllipsoidFixed(CurrentEcef);
    const FVector CandidateEcef = CurrentEcef + EastNorthUpToEcef.TransformVector(
        FVector(DeltaMeters.X, DeltaMeters.Y, 0.0));
    OutLongitudeLatitudeHeight =
        Ellipsoid->EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(CandidateEcef);
    return true;
}

bool UArriettyCesiumNavigationComponent::ApplyNavigationPose(
    const FVector2D& LocalPositionMeters,
    double HeightMeters,
    double HeadingDegrees,
    double PitchDegrees,
    double BankDegrees)
{
    FVector LongitudeLatitudeHeight;
    if (!PredictLongitudeLatitudeHeight(LocalPositionMeters, LongitudeLatitudeHeight))
    {
        return false;
    }
    LongitudeLatitudeHeight.Z = BaseEllipsoidHeightMeters + HeightMeters;
    GlobeAnchor->MoveToLongitudeLatitudeHeight(LongitudeLatitudeHeight);
    GlobeAnchor->SetEastSouthUpRotation(
        FRotator(PitchDegrees, -HeadingDegrees, -BankDegrees).Quaternion());
    LastLocalPositionMeters = LocalPositionMeters;
    return true;
}

bool UArriettyCesiumNavigationComponent::BuildRideSurfaceTrace(
    const FVector2D& LocalPositionMeters,
    FVector& OutStart,
    FVector& OutEnd) const
{
    FVector LongitudeLatitudeHeight;
    if (!Georeference ||
        !PredictLongitudeLatitudeHeight(LocalPositionMeters, LongitudeLatitudeHeight))
    {
        return false;
    }
    LongitudeLatitudeHeight.Z = BaseEllipsoidHeightMeters + 10000.0;
    OutStart = Georeference->TransformLongitudeLatitudeHeightPositionToUnreal(
        LongitudeLatitudeHeight);
    LongitudeLatitudeHeight.Z = BaseEllipsoidHeightMeters - 10000.0;
    OutEnd = Georeference->TransformLongitudeLatitudeHeightPositionToUnreal(
        LongitudeLatitudeHeight);
    return true;
}

double UArriettyCesiumNavigationComponent::HeightMetersFromWorldLocation(
    const FVector& WorldLocation) const
{
    if (!Georeference)
    {
        return Super::HeightMetersFromWorldLocation(WorldLocation);
    }
    return Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(WorldLocation).Z -
        BaseEllipsoidHeightMeters;
}

bool UArriettyCesiumNavigationComponent::GetGeospatialCoordinates(
    double& OutLongitudeDegrees,
    double& OutLatitudeDegrees,
    double& OutEllipsoidHeightMeters) const
{
    if (!bInitialized || !GlobeAnchor)
    {
        return false;
    }
    const FVector LongitudeLatitudeHeight = GlobeAnchor->GetLongitudeLatitudeHeight();
    OutLongitudeDegrees = LongitudeLatitudeHeight.X;
    OutLatitudeDegrees = LongitudeLatitudeHeight.Y;
    OutEllipsoidHeightMeters = LongitudeLatitudeHeight.Z;
    return true;
}
