// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyNavigationComponent.h"

UArriettyNavigationComponent::UArriettyNavigationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UArriettyNavigationComponent::InitializeNavigation(
    const FVector2D& LocalPositionMeters,
    double HeightMeters)
{
    return false;
}

bool UArriettyNavigationComponent::ApplyNavigationPose(
    const FVector2D& LocalPositionMeters,
    double HeightMeters,
    double HeadingDegrees,
    double PitchDegrees,
    double BankDegrees)
{
    return false;
}

bool UArriettyNavigationComponent::BuildRideSurfaceTrace(
    const FVector2D& LocalPositionMeters,
    FVector& OutStart,
    FVector& OutEnd) const
{
    return false;
}

double UArriettyNavigationComponent::HeightMetersFromWorldLocation(
    const FVector& WorldLocation) const
{
    return WorldLocation.Z / 100.0;
}

bool UArriettyNavigationComponent::GetGeospatialCoordinates(
    double& OutLongitudeDegrees,
    double& OutLatitudeDegrees,
    double& OutEllipsoidHeightMeters) const
{
    return false;
}
