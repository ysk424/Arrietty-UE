# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT
#Requires -Version 7.0

function Resolve-ArriettyProjectFile {
    param([Parameter(Mandatory)][string]$Project)

    $resolved = Get-Item -LiteralPath $Project -ErrorAction Stop
    if (-not $resolved.PSIsContainer) {
        if ($resolved.Extension -ne ".uproject") {
            throw "Project must be a .uproject file or a directory containing one: $Project"
        }
        return $resolved.FullName
    }

    $projects = @(Get-ChildItem -LiteralPath $resolved.FullName -File -Filter *.uproject)
    if ($projects.Count -ne 1) {
        throw "Project directory must contain exactly one .uproject file: $($resolved.FullName)"
    }
    return $projects[0].FullName
}

function Enable-ArriettyRuntimePlugin {
    param([Parameter(Mandatory)][string]$ProjectFile)

    $descriptor = Get-Content -LiteralPath $ProjectFile -Raw | ConvertFrom-Json
    $plugins = if ($null -eq $descriptor.Plugins) { @() } else { @($descriptor.Plugins) }
    $entry = $plugins | Where-Object { $_.Name -eq "ArriettyRuntime" } | Select-Object -First 1
    if ($null -eq $entry) {
        $plugins += [pscustomobject][ordered]@{ Name = "ArriettyRuntime"; Enabled = $true }
    }
    else {
        $entry.Enabled = $true
    }

    if ($null -eq $descriptor.PSObject.Properties["Plugins"]) {
        $descriptor | Add-Member -NotePropertyName Plugins -NotePropertyValue $plugins
    }
    else {
        $descriptor.Plugins = $plugins
    }
    $descriptor | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $ProjectFile -Encoding utf8
}

function Copy-ArriettyRuntimePlugin {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )

    $sourceRoot = [System.IO.Path]::GetFullPath($Source)
    $destinationRoot = [System.IO.Path]::GetFullPath($Destination)
    if ($sourceRoot.TrimEnd('\') -eq $destinationRoot.TrimEnd('\')) {
        return
    }
    if (-not (Test-Path -LiteralPath (Join-Path $sourceRoot "ArriettyRuntime.uplugin"))) {
        throw "ArriettyRuntime source plugin was not found: $sourceRoot"
    }

    New-Item -ItemType Directory -Force -Path $destinationRoot | Out-Null
    $manifestPath = Join-Path $destinationRoot ".arrietty-managed-files.txt"
    $oldFiles = if (Test-Path -LiteralPath $manifestPath) {
        @(Get-Content -LiteralPath $manifestPath | Where-Object { $_ })
    }
    else {
        @()
    }

    $sourceFiles = @(Get-ChildItem -LiteralPath $sourceRoot -File -Recurse | Where-Object {
        $relative = [System.IO.Path]::GetRelativePath($sourceRoot, $_.FullName)
        $firstPart = ($relative -split '[\\/]')[0]
        $firstPart -notin @("Binaries", "Intermediate", "Saved") -and
            $_.Name -ne ".arrietty-managed-files.txt"
    })
    $newFiles = @($sourceFiles | ForEach-Object {
        [System.IO.Path]::GetRelativePath($sourceRoot, $_.FullName).Replace('\', '/')
    })

    $destinationPrefix = $destinationRoot.TrimEnd('\') + '\'
    foreach ($relative in $oldFiles) {
        if ($relative -in $newFiles) {
            continue
        }
        $obsolete = [System.IO.Path]::GetFullPath((Join-Path $destinationRoot $relative))
        if (-not $obsolete.StartsWith($destinationPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Unsafe managed plugin path in manifest: $relative"
        }
        if (Test-Path -LiteralPath $obsolete -PathType Leaf) {
            Remove-Item -LiteralPath $obsolete -Force
        }
    }

    foreach ($file in $sourceFiles) {
        $relative = [System.IO.Path]::GetRelativePath($sourceRoot, $file.FullName)
        $target = Join-Path $destinationRoot $relative
        New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $target -Force
    }
    [System.IO.File]::WriteAllLines($manifestPath, [string[]]($newFiles | Sort-Object))
}

function Invoke-ArriettyEditorBuild {
    param(
        [Parameter(Mandatory)][string]$ProjectFile,
        [Parameter(Mandatory)][string]$EngineRoot
    )

    $projectName = [System.IO.Path]::GetFileNameWithoutExtension($ProjectFile)
    $dotnet = Join-Path $EngineRoot "Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe"
    $ubt = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
    $build = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
    foreach ($required in @($dotnet, $ubt, $build)) {
        if (-not (Test-Path -LiteralPath $required)) {
            throw "Required Unreal Engine build tool was not found: $required"
        }
    }

    & $dotnet $ubt -ProjectFiles "-Project=$ProjectFile" -Game -2026 -Progress
    if ($LASTEXITCODE -ne 0) {
        throw "Visual Studio 2026 solution generation failed with exit code $LASTEXITCODE"
    }
    & $build "${projectName}Editor" Win64 Development "-Project=$ProjectFile" `
        -WaitMutex -NoHotReloadFromIDE -2026
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal Editor build failed with exit code $LASTEXITCODE"
    }
}
