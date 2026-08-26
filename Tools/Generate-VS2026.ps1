# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT

param([string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8")

$ErrorActionPreference = "Stop"
$project = (Resolve-Path "$PSScriptRoot\..\Arrietty.uproject").Path
$dotnet = Join-Path $EngineRoot "Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe"
$ubt = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
& $dotnet $ubt -ProjectFiles "-Project=$project" -Game -2026 -Progress
if ($LASTEXITCODE -ne 0) {
    throw "Visual Studio 2026 solution generation failed"
}
