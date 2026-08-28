# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT
#Requires -Version 5.1

param(
    [ValidateSet("Build", "Upload", "Test")]
    [string]$Action = "Test",
    [string]$Port
)

$ErrorActionPreference = "Stop"
$sketch = (Resolve-Path "$PSScriptRoot\..\Hardware\ArriettyController").Path
$fqbn = "esp32:esp32:esp32"

function Resolve-ArriettyControllerPort {
    if ($Port) {
        return $Port
    }

    $device = Get-CimInstance Win32_PnPEntity |
        Where-Object { $_.Name -match 'USB-SERIAL CH340 \((COM[0-9]+)\)' } |
        Select-Object -First 1
    if (-not $device) {
        throw "USB-SERIAL CH340 was not found. Connect the Arrietty controller or pass -Port COMx."
    }
    if ($device.Name -notmatch '\((COM[0-9]+)\)') {
        throw "Could not parse the COM port from $($device.Name)"
    }
    return $Matches[1]
}

function Resolve-ArduinoCli {
    $cli = (Get-Command arduino-cli -ErrorAction SilentlyContinue).Source
    if (-not $cli) {
        $programFiles = if ($env:ProgramW6432) { $env:ProgramW6432 } else { $env:ProgramFiles }
        $cli = Join-Path $programFiles "Arduino CLI\arduino-cli.exe"
    }
    if (-not (Test-Path $cli)) {
        throw "Arduino CLI was not found. Install ArduinoSA.CLI with winget first."
    }
    return $cli
}

function Invoke-ControllerBuild {
    param([string]$Cli)

    & $Cli compile --fqbn $fqbn $sketch
    if ($LASTEXITCODE -ne 0) {
        throw "ESP32 controller build failed with exit code $LASTEXITCODE"
    }
}

if ($Action -eq "Build") {
    $cli = Resolve-ArduinoCli
    Invoke-ControllerBuild $cli
    exit 0
}

$resolvedPort = Resolve-ArriettyControllerPort
if ($Action -eq "Upload") {
    $cli = Resolve-ArduinoCli
    Invoke-ControllerBuild $cli
    & $cli upload --fqbn $fqbn --port $resolvedPort $sketch
    if ($LASTEXITCODE -ne 0) {
        throw "ESP32 controller upload failed with exit code $LASTEXITCODE"
    }
    Write-Host "Arrietty controller uploaded to $resolvedPort" -ForegroundColor Green
    exit 0
}

$serial = [System.IO.Ports.SerialPort]::new(
    $resolvedPort,
    115200,
    [System.IO.Ports.Parity]::None,
    8,
    [System.IO.Ports.StopBits]::One)
$serial.NewLine = "`n"
$serial.ReadTimeout = 500
$serial.WriteTimeout = 2000
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$packetPattern = [regex]'^A1,([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)$'

function Read-MatchingLine {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [scriptblock]$Accept,
        [int]$TimeoutSeconds = 5
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $line = $Serial.ReadLine().Trim()
            if (& $Accept $line) {
                return $line
            }
        } catch [System.TimeoutException] {
        }
    }
    throw "Timed out waiting for an Arrietty controller response"
}

try {
    Write-Host "Testing Arrietty controller on $resolvedPort"
    $serial.Open()
    Start-Sleep -Milliseconds 1500
    $null = $serial.ReadExisting()

    $serial.Write("PING`n")
    $pong = Read-MatchingLine $serial { param($line) $line -eq "PONG ARRIETTY-CONTROLLER/1" }
    Write-Host "PASS handshake: $pong" -ForegroundColor Green

    $serial.Write("READ`n")
    $packet = Read-MatchingLine $serial { param($line) $packetPattern.IsMatch($line) }
    $match = $packetPattern.Match($packet)
    for ($axisGroup = 2; $axisGroup -le 5; $axisGroup++) {
        $axis = [int]$match.Groups[$axisGroup].Value
        if ($axis -lt -32767 -or $axis -gt 32767) {
            throw "Axis value is outside the protocol range: $packet"
        }
    }
    $buttonMask = [int]$match.Groups[6].Value
    if ($buttonMask -lt 0 -or $buttonMask -gt 255) {
        throw "Button mask is outside the protocol range: $packet"
    }
    Write-Host "PASS state packet: $packet" -ForegroundColor Green

    $serial.Write("STREAM ON`n")
    $null = Read-MatchingLine $serial { param($line) $line -eq "OK STREAM ON 50HZ" }
    $previousSequence = -1L
    for ($sampleIndex = 0; $sampleIndex -lt 10; $sampleIndex++) {
        $streamPacket = Read-MatchingLine $serial { param($line) $packetPattern.IsMatch($line) }
        $streamMatch = $packetPattern.Match($streamPacket)
        $currentSequence = [long]$streamMatch.Groups[1].Value
        if ($currentSequence -le $previousSequence) {
            throw "Sequence number did not advance: $streamPacket"
        }
        $previousSequence = $currentSequence
    }
    $serial.Write("STREAM OFF`n")
    $null = Read-MatchingLine $serial { param($line) $line -eq "OK STREAM OFF" }
    Write-Host "PASS 50 Hz stream and sequence numbers" -ForegroundColor Green
    Write-Host "ARRIETTY CONTROLLER TEST PASSED" -ForegroundColor Green
} finally {
    if ($serial.IsOpen) {
        try { $serial.Write("STREAM OFF`n") } catch { }
        $serial.Close()
    }
    $serial.Dispose()
}
