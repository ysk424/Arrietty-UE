# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT

param(
    [Parameter(Mandatory=$true)][string]$TmuxTarget,
    [Parameter(Mandatory=$true)][string]$WslDistro,
    [int]$Port = 49000
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

# Make the launcher safe to re-run after a tmux/WSL restart. The Windows
# helper may still be alive but still point at the previous tmux pane.
$existingEndpoints = @(Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue)
foreach ($endpoint in $existingEndpoints) {
    $existingProcess = Get-Process -Id $endpoint.OwningProcess -ErrorAction Stop
    if ($existingProcess.ProcessName -ne "ArriettyVoiceBridge") {
        throw "UDP port $Port is already used by $($existingProcess.ProcessName) (PID $($existingProcess.Id))."
    }
    Stop-Process -Id $existingProcess.Id
    $existingProcess.WaitForExit(5000) | Out-Null
    Write-Host "Previous Arrietty Voice Bridge stopped (PID $($existingProcess.Id))."
}

& (Join-Path $root "build.ps1") -Configuration Release
$exe = Join-Path $root "bin\Release\ArriettyVoiceBridge.exe"
$logRoot = Join-Path $env:LOCALAPPDATA "ArriettyVoiceBridge\Logs"
$null = New-Item -ItemType Directory -Path $logRoot -Force
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$stdoutLog = Join-Path $logRoot "voice-bridge-$stamp.out.log"
$stderrLog = Join-Path $logRoot "voice-bridge-$stamp.err.log"
$process = Start-Process -FilePath $exe -ArgumentList @(
    "--tmux-target", $TmuxTarget,
    "--wsl-distro", $WslDistro,
    "--port", $Port
) -WorkingDirectory $root -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog -PassThru
Write-Host "Arrietty Voice Bridge started (PID $($process.Id), tmux $TmuxTarget)." -ForegroundColor Green
Write-Host "Voice Bridge logs: $stdoutLog / $stderrLog"
