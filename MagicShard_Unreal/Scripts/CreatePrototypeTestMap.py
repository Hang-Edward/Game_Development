import sys

import unreal


MAP_PATH = "/Game/Maps/PrototypeRuntimeTest"


def log(message):
    unreal.log("[MagicShardMapSetup] " + message)


def load_class(path):
    loaded = unreal.load_class(None, path)
    if loaded is None:
        raise RuntimeError("Unable to load class: " + path)
    return loaded


def spawn_actor(actor_class, location, rotation=(0.0, 0.0, 0.0), label=None):
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actor_subsystem is None:
        raise RuntimeError("EditorActorSubsystem is unavailable.")

    actor = actor_subsystem.spawn_actor_from_class(
        actor_class,
        unreal.Vector(*location),
        unreal.Rotator(*rotation),
    )
    if actor is None:
        raise RuntimeError("Failed to spawn actor: " + str(actor_class))
    if label:
        actor.set_actor_label(label)
    return actor


def set_static_mesh(actor, mesh_path):
    mesh = unreal.load_asset(mesh_path)
    if mesh is None:
        raise RuntimeError("Unable to load mesh: " + mesh_path)

    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if component is None:
        raise RuntimeError("Actor has no StaticMeshComponent: " + actor.get_name())

    component.set_static_mesh(mesh)
    component.set_collision_profile_name("BlockAll")
    return component


def configure_world():
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None:
        raise RuntimeError("No editor world is available.")
    return world


def clear_previous_generated_actors():
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actor_subsystem is None:
        return

    for actor in actor_subsystem.get_all_level_actors():
        label = actor.get_actor_label()
        if label.startswith("Prototype_") or label.startswith("PrototypeShardPickup"):
            actor_subsystem.destroy_actor(actor)


def create_level():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if level_editor is None:
        raise RuntimeError("LevelEditorSubsystem is unavailable.")

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        log("Opening existing level " + MAP_PATH)
        if not level_editor.load_level(MAP_PATH):
            raise RuntimeError("Failed to load level: " + MAP_PATH)
    else:
        log("Creating level " + MAP_PATH)
        if not level_editor.new_level(MAP_PATH):
            raise RuntimeError("Failed to create level: " + MAP_PATH)

    configure_world()

    floor = spawn_actor(unreal.StaticMeshActor, (0.0, 0.0, -55.0), label="Prototype_Static_Floor")
    floor.set_actor_scale3d(unreal.Vector(18.0, 18.0, 1.0))
    set_static_mesh(floor, "/Engine/BasicShapes/Cube.Cube")

    ramp_a = spawn_actor(unreal.StaticMeshActor, (420.0, 0.0, 20.0), (0.0, 0.0, 12.0), "Prototype_Ramp_A")
    ramp_a.set_actor_scale3d(unreal.Vector(5.0, 2.5, 0.35))
    set_static_mesh(ramp_a, "/Engine/BasicShapes/Cube.Cube")

    ramp_b = spawn_actor(unreal.StaticMeshActor, (-420.0, 220.0, 35.0), (0.0, 25.0, -10.0), "Prototype_Ramp_B")
    ramp_b.set_actor_scale3d(unreal.Vector(4.0, 2.0, 0.35))
    set_static_mesh(ramp_b, "/Engine/BasicShapes/Cube.Cube")

    spawn_actor(unreal.PlayerStart, (0.0, -450.0, 120.0), label="Prototype_PlayerStart")
    spawn_actor(unreal.DirectionalLight, (-300.0, -400.0, 600.0), (-45.0, -35.0, 0.0), "Prototype_Sun")

    builder_class = load_class("/Script/MagicShard.MagicShardPrototypeWorldBuilder")
    spawn_actor(builder_class, (0.0, 0.0, 0.0), label="PrototypeWorldBuilder_BeginPlay")

    pickup_class = load_class("/Script/MagicShard.MagicShardShardPickup")
    for index in range(4):
        x = -300.0 + index * 200.0
        spawn_actor(pickup_class, (x, 260.0, 90.0), label="PrototypeShardPickup_%d" % index)

    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError("Failed to save current level.")

    unreal.SystemLibrary.collect_garbage()
    log("Saved " + MAP_PATH)


try:
    create_level()
except Exception as exc:
    unreal.log_error("[MagicShardMapSetup] " + str(exc))
    sys.exit(1)
