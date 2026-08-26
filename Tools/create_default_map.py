"""Create the minimal cooked entry map used by the native runtime world builder."""

import unreal


MAP_PATH = "/Game/Maps/ArriettyWorld"

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    if not level_editor.load_level(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")
else:
    if not level_editor.new_level(MAP_PATH):
        raise RuntimeError(f"Could not create {MAP_PATH}")

if not level_editor.save_current_level():
    raise RuntimeError(f"Could not save {MAP_PATH}")

unreal.log(f"Arrietty entry map is ready: {MAP_PATH}")
