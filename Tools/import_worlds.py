# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: GPL-3.0-or-later

"""Import SourceAssets/BlenderExports into /Game/ImportedWorlds in Unreal Editor."""

from pathlib import Path

import unreal


project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source = project / "SourceAssets" / "BlenderExports"
files = sorted(source.glob("*.glb"))
if not files:
    raise RuntimeError(f"No GLB files found under {source}; run Export-BlenderWorlds.ps1 first")

tasks = []
for path in files:
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = f"/Game/ImportedWorlds/{path.stem}"
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    unreal.log(f"Imported {task.filename}: {list(task.imported_object_paths)}")
