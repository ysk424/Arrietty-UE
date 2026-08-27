# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT
#Requires -Version 7.0

param(
    [Parameter(Mandatory)][string]$Project,
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "ArriettyProjectTools.ps1")

$projectFile = Resolve-ArriettyProjectFile -Project $Project
$projectRoot = Split-Path $projectFile
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$sourcePlugin = Join-Path $repositoryRoot "Plugins\ArriettyRuntime"
$destinationPlugin = Join-Path $projectRoot "Plugins\ArriettyRuntime"

Copy-ArriettyRuntimePlugin -Source $sourcePlugin -Destination $destinationPlugin
Enable-ArriettyRuntimePlugin -ProjectFile $projectFile

if (-not $SkipBuild) {
    Invoke-ArriettyEditorBuild -ProjectFile $projectFile -EngineRoot $EngineRoot
}

$pluginDescriptor = Get-Content -LiteralPath (Join-Path $destinationPlugin "ArriettyRuntime.uplugin") -Raw | ConvertFrom-Json
Write-Host "Arrietty Runtime $($pluginDescriptor.VersionName) updated: $projectFile"
Write-Host "World Content and project Config were preserved."
