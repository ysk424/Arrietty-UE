# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [ValidateSet("Development", "Shipping")]
    [string]$Configuration = "Shipping",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$OutputDirectory = "$PSScriptRoot\..\Dist"
)

$ErrorActionPreference = "Stop"
$project = (Resolve-Path "$PSScriptRoot\..\Arrietty.uproject").Path
$runUat = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$archive = [System.IO.Path]::GetFullPath($OutputDirectory)

& $runUat BuildCookRun `
    "-project=$project" -noP4 -platform=Win64 `
    "-clientconfig=$Configuration" -build -cook -stage -pak -archive `
    "-archivedirectory=$archive" -utf8output

if ($LASTEXITCODE -ne 0) {
    throw "Arrietty package failed with exit code $LASTEXITCODE"
}

$stagedExe = Join-Path (Split-Path $project) "Saved\StagedBuilds\Windows\Arrietty\Binaries\Win64\Arrietty-Win64-Shipping.exe"
$archivedExe = Join-Path $archive "Windows\Arrietty\Binaries\Win64\Arrietty-Win64-Shipping.exe"
if (!(Test-Path $archivedExe)) {
    throw "Packaged executable was not created: $archivedExe"
}
if ((Test-Path $stagedExe) -and
    (Get-FileHash $stagedExe -Algorithm SHA256).Hash -ne
    (Get-FileHash $archivedExe -Algorithm SHA256).Hash) {
    throw "Archive verification failed: the packaged executable is stale"
}

Write-Host "Package ready and verified: $archive\Windows\Arrietty.exe"
