"""Create the Level-based Arrietty demo with explicit runtime Actors."""

import unreal


MAP_PATH = "/Game/Worlds/ArriettyDemo/ArriettyDemo"

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    if not level_editor.load_level(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")
else:
    if not level_editor.new_level(MAP_PATH):
        raise RuntimeError(f"Could not create {MAP_PATH}")

actor_editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_editor.get_all_level_actors()
if not any(isinstance(actor, unreal.ArriettyWorldBuilder) for actor in actors):
    actor_editor.spawn_actor_from_class(
        unreal.ArriettyWorldBuilder,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
if not any(isinstance(actor, unreal.ArriettyCourseStart) for actor in actors):
    # The generated oval starts at UE (X=0, Y=+320 m) and initially travels +X.
    actor_editor.spawn_actor_from_class(
        unreal.ArriettyCourseStart,
        unreal.Vector(0.0, 32000.0, 50.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )

if not level_editor.save_current_level():
    raise RuntimeError(f"Could not save {MAP_PATH}")

unreal.log(f"Arrietty entry map is ready: {MAP_PATH}")
