# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT

param(
    [string]$Blender = "C:\Users\azoo\git\build_windows_Release_x64_vc17_Release\bin\blender.exe",
    [string]$Source = "C:\Users\azoo\git\Arrietty\test_data",
    [string]$Destination = "$PSScriptRoot\..\SourceAssets\BlenderExports"
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$exporter = Join-Path $PSScriptRoot "export_blender_world.py"

Get-ChildItem $Source -Filter *.blend | ForEach-Object {
    $output = Join-Path $Destination ($_.BaseName + ".glb")
    Remove-Item -Force -ErrorAction SilentlyContinue $output
    & $Blender $_.FullName --background --python $exporter -- $output
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $output) -or (Get-Item $output).Length -eq 0) {
        throw "Blender export failed for $($_.FullName)"
    }
}
