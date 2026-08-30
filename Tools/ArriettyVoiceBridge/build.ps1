# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "Visual Studio Build Tools (vswhere.exe) was not found." }
$installation = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
if (-not $installation) { throw "Visual Studio Build Tools was not found." }
$csc = Join-Path $installation "MSBuild\Current\Bin\Roslyn\csc.exe"
$framework = Join-Path $env:WINDIR "Microsoft.NET\Framework64\v4.0.30319"
$output = Join-Path $root "bin\$Configuration"
New-Item -ItemType Directory -Force -Path $output | Out-Null

$arguments = @(
    "/nologo", "/noconfig", "/nostdlib+", "/target:exe", "/platform:anycpu",
    "/langversion:latest", "/warn:4", "/out:$(Join-Path $output 'ArriettyVoiceBridge.exe')"
)
if ($Configuration -eq "Release") { $arguments += "/optimize+" } else { $arguments += "/optimize-"; $arguments += "/debug:full" }
foreach ($reference in @("mscorlib.dll", "System.dll", "System.Core.dll", "System.Net.Http.dll", "System.Web.Extensions.dll")) {
    $arguments += "/reference:$(Join-Path $framework $reference)"
}
foreach ($source in @(
    "Program.cs", "BridgeOptions.cs", "VoiceBridgeProtocol.cs", "VoiceBridge.cs", "TmuxClient.cs",
    "SpeechTextCleaner.cs", "SelfTests.cs", "Services\MciAudioRecorder.cs",
    "Services\OpenAiAudioClient.cs", "Properties\AssemblyInfo.cs")) {
    $arguments += (Join-Path $root $source)
}
& $csc $arguments
if ($LASTEXITCODE -ne 0) { throw "Building ArriettyVoiceBridge failed." }

$exe = Join-Path $output "ArriettyVoiceBridge.exe"
& $exe --self-test
if ($LASTEXITCODE -ne 0) { throw "ArriettyVoiceBridge self-test failed." }
Write-Host "Build complete: $exe" -ForegroundColor Green
