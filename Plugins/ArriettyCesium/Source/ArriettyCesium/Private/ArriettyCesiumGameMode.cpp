// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

#include "ArriettyCesiumGameMode.h"

#include "ArriettyCesiumPawn.h"

AArriettyCesiumGameMode::AArriettyCesiumGameMode()
{
    DefaultPawnClass = AArriettyCesiumPawn::StaticClass();
}
