# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT

"""Export the currently opened Blender world to one Unreal-friendly GLB."""

from pathlib import Path
import sys

import bpy


def argument_after_separator() -> Path:
    if "--" not in sys.argv:
        raise SystemExit("usage: blender world.blend --background --python export_blender_world.py -- output.glb")
    arguments = sys.argv[sys.argv.index("--") + 1 :]
    if len(arguments) != 1:
        raise SystemExit("exactly one output .glb path is required")
    return Path(arguments[0]).resolve()


output = argument_after_separator()
output.parent.mkdir(parents=True, exist_ok=True)

for obj in bpy.context.scene.objects:
    obj.select_set(obj.type in {"MESH", "CURVE", "FONT", "SURFACE", "META"})

options = {
    "filepath": str(output),
    "export_format": "GLB",
    "export_yup": True,
    "export_apply": True,
    "export_extras": True,
    "export_cameras": False,
    "export_lights": True,
    "use_selection": True,
}

try:
    bpy.ops.preferences.addon_enable(module="io_scene_gltf2")
except RuntimeError:
    pass

try:
    bpy.ops.export_scene.gltf.get_rna_type()
except AttributeError:
    result = bpy.ops.wm.gltf_export(**options)
else:
    result = bpy.ops.export_scene.gltf(**options)

if "FINISHED" not in result:
    raise SystemExit(f"GLB export failed: {result}")
print(f"Arrietty world exported: {output}")
