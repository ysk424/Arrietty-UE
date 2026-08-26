# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$PackageRoot = "$PSScriptRoot\..\Dist\Windows",
    [switch]$UseHmd,
    [ValidateRange(1, 60)]
    [int]$DurationSeconds = 5
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path $PackageRoot).Path
$launcher = Join-Path $root "Arrietty.exe"

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class ArriettyWinMessage
{
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
}
"@

$arguments = @("-NoSound", "-Windowed", "-ResX=960", "-ResY=540")
if (-not $UseHmd) {
    $arguments = @("-NoHMD") + $arguments
}

$bootstrap = Start-Process $launcher `
    -ArgumentList $arguments `
    -WorkingDirectory $root `
    -PassThru

try {
    Start-Sleep -Seconds $DurationSeconds
    $target = Get-Process "Arrietty-Win64-Shipping" -ErrorAction SilentlyContinue |
        Where-Object { $_.Path -like "$root*" } |
        Select-Object -First 1
    if (-not $target -or $target.MainWindowHandle -eq 0) {
        throw "Arrietty window was not created"
    }

    [ArriettyWinMessage]::PostMessage($target.MainWindowHandle, 0x0100, [IntPtr]0x1B, [IntPtr]0) | Out-Null
    [ArriettyWinMessage]::PostMessage($target.MainWindowHandle, 0x0101, [IntPtr]0x1B, [IntPtr]0) | Out-Null
    if (-not $target.WaitForExit(10000)) {
        throw "Esc did not close Arrietty within 10 seconds"
    }
    $bootstrap.WaitForExit(5000) | Out-Null
    $mode = "NoHMD"
    if ($UseHmd) {
        $mode = "OpenXR"
    }
    Write-Host "Packaged Esc exit verified in $mode mode."
}
finally {
    Get-Process "Arrietty", "Arrietty-Win64-Shipping" -ErrorAction SilentlyContinue |
        Where-Object { $_.Path -like "$root*" } |
        Stop-Process -Force
}
