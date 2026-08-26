# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT

param(
    [ValidateSet("Editor", "Game", "Shipping")]
    [string]$Target = "Editor",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"
$project = (Resolve-Path "$PSScriptRoot\..\Arrietty.uproject").Path
$build = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"

switch ($Target) {
    "Editor"   { $targetName = "ArriettyEditor"; $configuration = "Development" }
    "Game"     { $targetName = "Arrietty";       $configuration = "Development" }
    "Shipping" { $targetName = "Arrietty";       $configuration = "Shipping" }
}

& $build $targetName Win64 $configuration "-Project=$project" -WaitMutex -NoHotReloadFromIDE -2026
if ($LASTEXITCODE -ne 0) {
    throw "Unreal build failed with exit code $LASTEXITCODE"
}
