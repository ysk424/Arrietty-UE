# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT
#Requires -Version 7.0

$ErrorActionPreference = "Stop"
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) `
    ("ArriettyWorldProjectTest-" + [guid]::NewGuid().ToString("N"))
$projectName = "ArriettyToolTest"
$projectRoot = Join-Path $testRoot $projectName
$projectFile = Join-Path $projectRoot "$projectName.uproject"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw "World project tool test failed: $Message"
    }
}

try {
    & (Join-Path $PSScriptRoot "New-ArriettyWorldProject.ps1") `
        -Name $projectName -DestinationRoot $testRoot -SkipBuild

    Assert-True (Test-Path -LiteralPath $projectFile) ".uproject was not created"
    Assert-True (Test-Path -LiteralPath (Join-Path $projectRoot "Content\Worlds\ArriettyDemo\ArriettyDemo.umap")) `
        "demo Level was not copied"
    Assert-True (Test-Path -LiteralPath (Join-Path $projectRoot "Plugins\ArriettyRuntime\ArriettyRuntime.uplugin")) `
        "runtime plugin was not copied"
    Assert-True (Test-Path -LiteralPath (Join-Path $projectRoot "Source\${projectName}Editor.Target.cs")) `
        "named Editor target was not generated"

    $descriptor = Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json
    Assert-True ($null -ne ($descriptor.Modules | Where-Object { $_.Name -eq "ArriettyWorldHost" })) `
        "host module is missing"
    Assert-True ($null -ne ($descriptor.Plugins | Where-Object { $_.Name -eq "ArriettyRuntime" -and $_.Enabled })) `
        "runtime plugin is not enabled"

    $worldMarker = Join-Path $projectRoot "Content\UserWorld.marker"
    $configHashBefore = (Get-FileHash (Join-Path $projectRoot "Config\DefaultEngine.ini") -Algorithm SHA256).Hash
    "preserve me" | Set-Content -LiteralPath $worldMarker -Encoding utf8
    & (Join-Path $PSScriptRoot "Update-ArriettyWorldProject.ps1") `
        -Project $projectFile -SkipBuild

    Assert-True (Test-Path -LiteralPath $worldMarker) "world Content was changed by the updater"
    $configHashAfter = (Get-FileHash (Join-Path $projectRoot "Config\DefaultEngine.ini") -Algorithm SHA256).Hash
    Assert-True ($configHashBefore -eq $configHashAfter) "project Config was changed by the updater"
    Write-Host "World project creation/update tests passed."
}
finally {
    $tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    $testRootFull = [System.IO.Path]::GetFullPath($testRoot)
    if ($testRootFull.StartsWith($tempRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path $testRootFull -Leaf).StartsWith("ArriettyWorldProjectTest-")) {
        Remove-Item -LiteralPath $testRootFull -Recurse -Force -ErrorAction SilentlyContinue
    }
}
