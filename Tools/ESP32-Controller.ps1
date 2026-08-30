# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT
#Requires -Version 5.1

param(
    [ValidateSet("Build", "Upload", "Test", "ButtonTest", "AudioTest")]
    [string]$Action = "Test",
    [string]$Port,
    [ValidateRange(5, 120)]
    [int]$ButtonTimeoutSeconds = 30,
    [string]$ButtonSequence = "1,2,3,4,5,6"
)

$ErrorActionPreference = "Stop"
$buttonNumbers = @()
foreach ($buttonText in $ButtonSequence.Split(',')) {
    $parsedButton = 0
    if (-not [int]::TryParse($buttonText.Trim(), [ref]$parsedButton) -or
        $parsedButton -lt 1 -or $parsedButton -gt 6) {
        throw "ButtonSequence must be a comma-separated list containing Button numbers 1 through 6."
    }
    if ($buttonNumbers -contains $parsedButton) {
        throw "ButtonSequence contains Button $parsedButton more than once."
    }
    $buttonNumbers += $parsedButton
}
if ($buttonNumbers.Count -eq 0) {
    throw "ButtonSequence must contain at least one Button number."
}
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
$testPacketPattern = [regex]'^T1,([0-9]+),([0-9]+),([0-9]+),([0-9]+)$'

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

function Read-TestState {
    param([System.IO.Ports.SerialPort]$Serial)

    $Serial.Write("READ`n")
    $packet = Read-MatchingLine $Serial { param($line) $testPacketPattern.IsMatch($line) } 2
    $match = $testPacketPattern.Match($packet)
    $state = [pscustomobject]@{
        Packet = $packet
        Sequence = [long]$match.Groups[1].Value
        RawMask = [int]$match.Groups[2].Value
        StableMask = [int]$match.Groups[3].Value
        SeenMask = [int]$match.Groups[4].Value
    }
    foreach ($maskName in @("RawMask", "StableMask", "SeenMask")) {
        if ($state.$maskName -lt 0 -or $state.$maskName -gt 63) {
            throw "Test-mode $maskName is outside the Button 1-6 range: $packet"
        }
    }
    return $state
}

function Wait-AllButtonsReleased {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [int]$TimeoutSeconds
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $releasedSince = $null
    while ([DateTime]::UtcNow -lt $deadline) {
        $state = Read-TestState $Serial
        if ($state.RawMask -eq 0 -and $state.StableMask -eq 0) {
            if ($null -eq $releasedSince) {
                $releasedSince = [DateTime]::UtcNow
            } elseif (([DateTime]::UtcNow - $releasedSince).TotalMilliseconds -ge 300) {
                return
            }
        } else {
            $releasedSince = $null
        }
        Start-Sleep -Milliseconds 20
    }
    throw "Button release timed out. Release all Button 1-6 inputs and retry."
}

function Invoke-SingleTestTone {
    param(
        [int]$Frequency,
        [int]$DurationMilliseconds
    )

    if (-not ("ArriettyButtonTestAudio" -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Media;
using System.Text;

public static class ArriettyButtonTestAudio
{
    public static void PlayTone(int frequency, int durationMilliseconds)
    {
        const int sampleRate = 44100;
        const short channels = 1;
        const short bitsPerSample = 16;
        int sampleCount = Math.Max(1, sampleRate * durationMilliseconds / 1000);
        int dataLength = sampleCount * channels * (bitsPerSample / 8);
        MemoryStream stream = new MemoryStream(44 + dataLength);
        BinaryWriter writer = new BinaryWriter(stream, Encoding.ASCII);
        writer.Write(Encoding.ASCII.GetBytes("RIFF"));
        writer.Write(36 + dataLength);
        writer.Write(Encoding.ASCII.GetBytes("WAVE"));
        writer.Write(Encoding.ASCII.GetBytes("fmt "));
        writer.Write(16);
        writer.Write((short)1);
        writer.Write(channels);
        writer.Write(sampleRate);
        writer.Write(sampleRate * channels * (bitsPerSample / 8));
        writer.Write((short)(channels * (bitsPerSample / 8)));
        writer.Write(bitsPerSample);
        writer.Write(Encoding.ASCII.GetBytes("data"));
        writer.Write(dataLength);

        int rampSamples = Math.Max(1, sampleRate / 200);
        for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
        {
            double attack = Math.Min(1.0, (double)sampleIndex / rampSamples);
            double release = Math.Min(1.0, (double)(sampleCount - 1 - sampleIndex) / rampSamples);
            double envelope = Math.Min(attack, release);
            short sample = (short)(Math.Sin(2.0 * Math.PI * frequency * sampleIndex / sampleRate) *
                12000.0 * envelope);
            writer.Write(sample);
        }
        writer.Flush();
        stream.Position = 0;
        using (SoundPlayer player = new SoundPlayer(stream))
        {
            player.Load();
            player.PlaySync();
        }
        writer.Dispose();
    }
}
'@
    }

    try {
        [ArriettyButtonTestAudio]::PlayTone($Frequency, $DurationMilliseconds)
    } catch {
        [System.Media.SystemSounds]::Exclamation.Play()
        Start-Sleep -Milliseconds $DurationMilliseconds
    }
}

function Invoke-ButtonTestTone {
    param([ValidateSet("Ready", "Pass", "Complete", "Failure")][string]$Signal)

    switch ($Signal) {
        "Ready" {
            Invoke-SingleTestTone 880 250
        }
        "Pass" {
            Invoke-SingleTestTone 1320 120
            Start-Sleep -Milliseconds 80
            Invoke-SingleTestTone 1560 120
        }
        "Complete" {
            Invoke-SingleTestTone 1040 120
            Start-Sleep -Milliseconds 70
            Invoke-SingleTestTone 1320 120
            Start-Sleep -Milliseconds 70
            Invoke-SingleTestTone 1760 180
        }
        "Failure" {
            for ($toneIndex = 0; $toneIndex -lt 3; $toneIndex++) {
                Invoke-SingleTestTone 330 250
                if ($toneIndex -lt 2) {
                    Start-Sleep -Milliseconds 100
                }
            }
        }
    }
}

if ($Action -eq "AudioTest") {
    Write-Host "Audio test: READY (one tone)"
    Invoke-ButtonTestTone Ready
    Start-Sleep -Milliseconds 500
    Write-Host "Audio test: PASS (two rising tones)"
    Invoke-ButtonTestTone Pass
    Start-Sleep -Milliseconds 500
    Write-Host "Audio test: COMPLETE (three rising tones)"
    Invoke-ButtonTestTone Complete
    $serial.Dispose()
    exit 0
}

try {
    Write-Host "Testing Arrietty controller on $resolvedPort"
    $serial.Open()
    Start-Sleep -Milliseconds 1500
    $null = $serial.ReadExisting()

    $serial.Write("PING`n")
    $pong = Read-MatchingLine $serial { param($line) $line -eq "PONG ARRIETTY-CONTROLLER/1" }
    Write-Host "PASS handshake: $pong" -ForegroundColor Green

    if ($Action -eq "ButtonTest") {
        $serial.Write("MODE TEST`n")
        $null = Read-MatchingLine $serial { param($line) $line -eq "OK MODE TEST BUTTONS 1-6" }
        Write-Host "TEST MODE entered. Release all Button 1-6 inputs." -ForegroundColor Cyan
        Wait-AllButtonsReleased $serial $ButtonTimeoutSeconds

        foreach ($buttonNumber in $buttonNumbers) {
            $expectedBit = 1 -shl ($buttonNumber - 1)
            $serial.Write("TEST RESET`n")
            $null = Read-MatchingLine $serial { param($line) $line -eq "OK TEST RESET" }
            Write-Host "Press Button $buttonNumber once; the READY tone repeats every 5 seconds..." -ForegroundColor Yellow

            $pressedDeadline = [DateTime]::UtcNow.AddSeconds($ButtonTimeoutSeconds)
            $nextReadyToneAt = [DateTime]::UtcNow
            $pressed = $false
            while ([DateTime]::UtcNow -lt $pressedDeadline) {
                if ([DateTime]::UtcNow -ge $nextReadyToneAt) {
                    Invoke-ButtonTestTone Ready
                    $nextReadyToneAt = [DateTime]::UtcNow.AddSeconds(5)
                }
                $state = Read-TestState $serial
                $unexpectedSeen = $state.SeenMask -band ((-bnot $expectedBit) -band 0x3f)
                if ($unexpectedSeen -ne 0) {
                    throw "Unexpected button input while testing Button $buttonNumber`: seen mask $($state.SeenMask) ($($state.Packet))"
                }
                $expectedWasSeen = ($state.SeenMask -band $expectedBit) -ne 0
                # TEST mode latches even a short raw edge in SeenMask. Requiring
                # the button to remain physically held until the PC reads a
                # packet made a valid tap fail solely because of test timing.
                if ($expectedWasSeen) {
                    $pressed = $true
                    break
                }
                Start-Sleep -Milliseconds 20
            }
            if (-not $pressed) {
                throw "Button $buttonNumber was not detected within $ButtonTimeoutSeconds seconds."
            }
            Write-Host "PASS Button $buttonNumber (GPIO $(@(18, 19, 21, 22, 23, 26)[$buttonNumber - 1]))" -ForegroundColor Green
            Invoke-ButtonTestTone Pass
            Write-Host "Release Button $buttonNumber..." -ForegroundColor Yellow
            Wait-AllButtonsReleased $serial $ButtonTimeoutSeconds
        }

        $serial.Write("MODE NORMAL`n")
        $null = Read-MatchingLine $serial { param($line) $line -eq "OK MODE NORMAL" }
        Write-Host "PASS Button $ButtonSequence hardware test; NORMAL MODE restored" -ForegroundColor Green
        Write-Host "ARRIETTY BUTTON TEST PASSED" -ForegroundColor Green
        Invoke-ButtonTestTone Complete
    } else {
        $serial.Write("MODE NORMAL`n")
        $null = Read-MatchingLine $serial { param($line) $line -eq "OK MODE NORMAL" }

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

        $serial.Write("MODE TEST`n")
        $null = Read-MatchingLine $serial { param($line) $line -eq "OK MODE TEST BUTTONS 1-6" }
        $testState = Read-TestState $serial
        Write-Host "PASS integrated test-mode packet: $($testState.Packet)" -ForegroundColor Green
        $serial.Write("TEST RESET`n")
        $null = Read-MatchingLine $serial { param($line) $line -eq "OK TEST RESET" }
        $serial.Write("MODE NORMAL`n")
        $null = Read-MatchingLine $serial { param($line) $line -eq "OK MODE NORMAL" }

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
    }
} catch {
    if ($Action -eq "ButtonTest") {
        Invoke-ButtonTestTone Failure
    }
    throw
} finally {
    if ($serial.IsOpen) {
        try { $serial.Write("STREAM OFF`n") } catch { }
        try { $serial.Write("MODE NORMAL`n") } catch { }
        $serial.Close()
    }
    $serial.Dispose()
}
