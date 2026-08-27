# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT
#Requires -Version 7.0

param(
    [Parameter(Mandatory)]
    [ValidatePattern('^[A-Za-z][A-Za-z0-9_]*$')]
    [string]$Name,
    [string]$DestinationRoot = (Join-Path ([Environment]::GetFolderPath("MyDocuments")) "Unreal Projects"),
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [switch]$SkipBuild,
    [switch]$Open
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "ArriettyProjectTools.ps1")

$reservedNames = @("CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9")
if ($Name.ToUpperInvariant() -in $reservedNames) {
    throw "Project name is reserved by Windows: $Name"
}

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$destinationRootFull = [System.IO.Path]::GetFullPath($DestinationRoot)
$projectRoot = Join-Path $destinationRootFull $Name
$projectFile = Join-Path $projectRoot "$Name.uproject"
if (Test-Path -LiteralPath $projectRoot) {
    throw "Destination already exists; no files were changed: $projectRoot"
}

$sourcePlugin = Join-Path $repositoryRoot "Plugins\ArriettyRuntime"
$sourceDemo = Join-Path $repositoryRoot "Content\Worlds\ArriettyDemo"
if (-not (Test-Path -LiteralPath (Join-Path $sourceDemo "ArriettyDemo.umap"))) {
    throw "The Arrietty demo Level is missing: $sourceDemo"
}

New-Item -ItemType Directory -Force -Path $projectRoot | Out-Null
try {
    foreach ($directory in @("Config", "Content\Worlds\ArriettyDemo", "Plugins", "Source")) {
        New-Item -ItemType Directory -Force -Path (Join-Path $projectRoot $directory) | Out-Null
    }

    foreach ($file in @("DefaultEngine.ini", "DefaultGame.ini", "DefaultInput.ini")) {
        Copy-Item -LiteralPath (Join-Path $repositoryRoot "Config\$file") `
            -Destination (Join-Path $projectRoot "Config\$file")
    }
    $projectId = [guid]::NewGuid().ToString("N").ToUpperInvariant()
    $defaultEnginePath = Join-Path $projectRoot "Config\DefaultEngine.ini"
    $defaultEngine = Get-Content -LiteralPath $defaultEnginePath -Raw
    $defaultEngine = $defaultEngine -replace '(?m)^ProjectID=.*$', "ProjectID=$projectId"
    $defaultEngine = $defaultEngine -replace '(?m)^ProjectName=Arrietty$', "ProjectName=$Name"
    $defaultEngine | Set-Content -LiteralPath $defaultEnginePath -Encoding utf8

    $defaultGamePath = Join-Path $projectRoot "Config\DefaultGame.ini"
    $defaultGame = Get-Content -LiteralPath $defaultGamePath -Raw
    $defaultGame = $defaultGame -replace '(?m)^ProjectName=Arrietty$', "ProjectName=$Name"
    $defaultGame | Set-Content -LiteralPath $defaultGamePath -Encoding utf8

    Copy-Item -LiteralPath (Join-Path $sourceDemo "ArriettyDemo.umap") `
        -Destination (Join-Path $projectRoot "Content\Worlds\ArriettyDemo\ArriettyDemo.umap")
    Copy-Item -LiteralPath (Join-Path $repositoryRoot "Source\ArriettyWorldHost") `
        -Destination (Join-Path $projectRoot "Source") -Recurse
    $hostSourcePath = Join-Path $projectRoot "Source\ArriettyWorldHost\Private\ArriettyWorldHost.cpp"
    $hostSource = Get-Content -LiteralPath $hostSourcePath -Raw
    $hostSource = $hostSource.Replace('ArriettyWorldHost, "Arrietty");', "ArriettyWorldHost, `"$Name`");")
    $hostSource | Set-Content -LiteralPath $hostSourcePath -Encoding utf8

    foreach ($kind in @("", "Editor")) {
        $sourceTargetName = if ($kind) { "ArriettyEditor.Target.cs" } else { "Arrietty.Target.cs" }
        $targetName = "${Name}${kind}"
        $targetSource = Get-Content -LiteralPath (Join-Path $repositoryRoot "Source\$sourceTargetName") -Raw
        $sourceClass = if ($kind) { "ArriettyEditorTarget" } else { "ArriettyTarget" }
        $targetSource = $targetSource.Replace($sourceClass, "${targetName}Target")
        $targetSource | Set-Content -LiteralPath (Join-Path $projectRoot "Source\${targetName}.Target.cs") -Encoding utf8
    }

    $descriptor = [ordered]@{
        FileVersion = 3
        EngineAssociation = "5.8"
        Category = "VR"
        Description = "An Arrietty world project named $Name."
        Modules = @([ordered]@{
            Name = "ArriettyWorldHost"
            Type = "Runtime"
            LoadingPhase = "Default"
        })
        Plugins = @(
            [ordered]@{ Name = "ArriettyRuntime"; Enabled = $true },
            [ordered]@{ Name = "OpenXR"; Enabled = $true },
            [ordered]@{ Name = "AndroidFileServer"; Enabled = $false }
        )
    }
    $descriptor | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $projectFile -Encoding utf8

    foreach ($file in @(".gitignore", ".gitattributes", ".vsconfig")) {
        Copy-Item -LiteralPath (Join-Path $repositoryRoot $file) -Destination (Join-Path $projectRoot $file)
    }
    Copy-ArriettyRuntimePlugin -Source $sourcePlugin `
        -Destination (Join-Path $projectRoot "Plugins\ArriettyRuntime")
}
catch {
    Write-Warning "Project creation stopped with a partial directory at: $projectRoot"
    throw
}

if (-not $SkipBuild) {
    Invoke-ArriettyEditorBuild -ProjectFile $projectFile -EngineRoot $EngineRoot
}

Write-Host "Arrietty world project ready: $projectFile"
Write-Host "Open ArriettyDemo or create a Level, place an ArriettyCourseStart, then use Play > VR Preview."
if ($Open) {
    Start-Process -FilePath $projectFile
}
