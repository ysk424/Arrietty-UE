# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT
#Requires -Version 7.0

param([string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8")

$ErrorActionPreference = "Stop"
$project = (Resolve-Path "$PSScriptRoot\..\Arrietty.uproject").Path
$editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
& $editor $project -unattended -NullRHI -NoSound `
    '-ExecCmds=Automation RunTests Arrietty;Quit' `
    '-TestExit=Automation Test Queue Empty' -log
if ($LASTEXITCODE -ne 0) {
    throw "Arrietty automation tests failed with exit code $LASTEXITCODE"
}

& (Join-Path $PSScriptRoot "Test-WorldProjectTools.ps1")
