import sys

import unreal


MAP_PATH = "/Game/Maps/Map01_Forest"
MAP_MESH_PATH = "/Game/Imported/Map/map_01_forest.map_01_forest"


def log(message):
    unreal.log("[MagicShardWorldSetup] " + message)


def fail(message):
    unreal.log_error("[MagicShardWorldSetup] " + message)
    raise RuntimeError(message)


def spawn_actor(actor_class, location, rotation=(0.0, 0.0, 0.0), label=None):
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actor_subsystem is None:
        fail("EditorActorSubsystem is unavailable.")

    actor = actor_subsystem.spawn_actor_from_class(
        actor_class,
        unreal.Vector(*location),
        unreal.Rotator(*rotation),
    )
    if actor is None:
        fail("Failed to spawn actor: " + str(actor_class))
    if label:
        actor.set_actor_label(label)
    return actor


def setup_static_mesh_actor(actor, mesh):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if component is None:
        fail("StaticMeshActor has no StaticMeshComponent.")

    # 启用网格三角形作为碰撞几何体，防止角色穿透地形
    try:
        body_setup = mesh.get_editor_property("body_setup")
        if body_setup is not None:
            body_setup.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            log("Enabled complex collision as simple on map mesh")
    except Exception as exc:
        log("Collision setup warning: " + str(exc))

    component.set_static_mesh(mesh)
    component.set_collision_profile_name("BlockAll")


def clear_generated_actors():
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actor_subsystem is None:
        return

    generated_labels = [
        "Imported_Map01_Forest",
        "PlayerStart",
        "Sun",
        "SkyLight",
        "SkyAtmosphere",
        "HeightFog",
    ]
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() in generated_labels:
            actor_subsystem.destroy_actor(actor)


def set_light_intensity(actor, component_class, intensity):
    component = actor.get_component_by_class(component_class)
    if component is None:
        return
    try:
        component.set_editor_property("intensity", intensity)
    except Exception as exc:
        log("Unable to set intensity on " + actor.get_actor_label() + ": " + str(exc))


def main():
    mesh = unreal.load_asset(MAP_MESH_PATH)
    if mesh is None:
        fail("Map mesh not found: " + MAP_MESH_PATH)

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if level_editor is None:
        fail("LevelEditorSubsystem is unavailable.")

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        log("Opening existing level " + MAP_PATH)
        if not level_editor.load_level(MAP_PATH):
            fail("Unable to load " + MAP_PATH)
    else:
        log("Creating level " + MAP_PATH)
        if not level_editor.new_level(MAP_PATH):
            fail("Unable to create " + MAP_PATH)

    clear_generated_actors()

    map_actor = spawn_actor(
        unreal.StaticMeshActor,
        (0.0, 0.0, 0.0),
        label="Imported_Map01_Forest",
    )
    setup_static_mesh_actor(map_actor, mesh)

    spawn_actor(unreal.PlayerStart, (0.0, -450.0, 800.0), label="PlayerStart")

    sun = spawn_actor(
        unreal.DirectionalLight,
        (-300.0, -400.0, 600.0),
        (-45.0, -35.0, 0.0),
        "Sun",
    )
    set_light_intensity(sun, unreal.DirectionalLightComponent, 5.0)

    skylight = spawn_actor(unreal.SkyLight, (0.0, 0.0, 500.0), label="SkyLight")
    set_light_intensity(skylight, unreal.SkyLightComponent, 1.5)

    spawn_actor(unreal.SkyAtmosphere, (0.0, 0.0, 0.0), label="SkyAtmosphere")
    fog = spawn_actor(unreal.ExponentialHeightFog, (0.0, 0.0, 0.0), label="HeightFog")
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_component is not None:
        try:
            fog_component.set_editor_property("fog_density", 0.01)
            fog_component.set_editor_property("fog_height_falloff", 0.15)
        except Exception as exc:
            log("Unable to tune fog: " + str(exc))

    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        fail("Failed to save current level.")

    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    log("Saved " + MAP_PATH)


try:
    main()
except Exception as exc:
    unreal.log_error("[MagicShardWorldSetup] " + str(exc))
    sys.exit(1)
