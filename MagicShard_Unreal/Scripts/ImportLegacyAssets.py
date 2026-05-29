import os
import sys

import unreal


ROOT = r"D:\VScode Projects\Game_Development"
CHARACTER_SOURCE = os.path.join(ROOT, "MagicShard_Unity", "Assets", "Models", "Character")
MAP_SOURCE = os.path.join(ROOT, "MagicShard_Unity", "Assets", "Models", "Map")

IDLE_FBX = os.path.join(CHARACTER_SOURCE, "stand.fbx")
WALK_FBX = os.path.join(CHARACTER_SOURCE, "walk.fbx")
RUN_FBX = os.path.join(CHARACTER_SOURCE, "run.fbx")
MAP_FBX = os.path.join(MAP_SOURCE, "map_01_forest.fbx")

CHARACTER_DEST = "/Game/Imported/Character"
MAP_DEST = "/Game/Imported/Map"


def log(message):
    unreal.log("[MagicShardImport] " + message)


def fail(message):
    unreal.log_error("[MagicShardImport] " + message)
    raise RuntimeError(message)


def ensure_file(path):
    if not os.path.exists(path):
        fail("Missing source file: " + path)


def make_task(filename, destination_path, options):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    return task


def import_tasks(tasks):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tools.import_asset_tasks(tasks)
    for task in tasks:
        log("Imported from " + task.get_editor_property("filename"))
        for asset_path in task.get_editor_property("imported_object_paths"):
            log("  " + asset_path)


def make_skeletal_options(skeleton=None, import_mesh=True, import_animations=True, animation_name=None, import_materials=False):
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_mesh", import_mesh)
    options.set_editor_property("import_animations", import_animations)
    options.set_editor_property("import_materials", import_materials)
    options.set_editor_property("import_textures", False)
    if skeleton is not None:
        options.set_editor_property("skeleton", skeleton)
    if animation_name:
        options.set_editor_property("override_animation_name", animation_name)

    skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
    skeletal_data.set_editor_property("import_uniform_scale", 1.0)
    skeletal_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)

    anim_data = options.get_editor_property("anim_sequence_import_data")
    anim_data.set_editor_property("import_uniform_scale", 1.0)
    return options


def make_static_options(import_materials=False):
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_materials", import_materials)
    options.set_editor_property("import_textures", False)

    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", True)
    static_data.set_editor_property("import_uniform_scale", 1.0)
    static_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    # 生成碰撞几何体，否则角色会穿透地形坠落
    static_data.set_editor_property("auto_generate_collision", True)
    return options


def find_first_asset(asset_class, search_path):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(search_path, recursive=True)
    for data in assets:
        asset = data.get_asset()
        if isinstance(asset, asset_class):
            return asset
    return None


def import_character():
    if find_first_asset(unreal.SkeletalMesh, CHARACTER_DEST) is None:
        log("Importing character Idle mesh and animation")
        import_tasks([
            make_task(IDLE_FBX, CHARACTER_DEST, make_skeletal_options(import_mesh=True, import_animations=True, animation_name="MS_Idle", import_materials=True))
        ])
    else:
        log("Character skeletal mesh already exists; keeping it and importing missing animations")

    skeleton = find_first_asset(unreal.Skeleton, CHARACTER_DEST)
    if skeleton is None:
        fail("Imported character skeleton was not found under " + CHARACTER_DEST)
    log("Using skeleton: " + skeleton.get_path_name())

    log("Importing Walk/Run animations with the shared skeleton")
    import_tasks([
        make_task(WALK_FBX, CHARACTER_DEST, make_skeletal_options(skeleton=skeleton, import_mesh=False, import_animations=True, animation_name="MS_Walk")),
        make_task(RUN_FBX, CHARACTER_DEST, make_skeletal_options(skeleton=skeleton, import_mesh=False, import_animations=True, animation_name="MS_Run")),
    ])


def import_map():
    log("Importing forest map")
    import_tasks([
        make_task(MAP_FBX, MAP_DEST, make_static_options(import_materials=True))
    ])


def main():
    for path in [IDLE_FBX, WALK_FBX, RUN_FBX, MAP_FBX]:
        ensure_file(path)

    import_character()
    import_map()

    unreal.EditorAssetLibrary.save_directory(CHARACTER_DEST, only_if_is_dirty=False, recursive=True)
    unreal.EditorAssetLibrary.save_directory(MAP_DEST, only_if_is_dirty=False, recursive=True)
    log("Import complete")


try:
    main()
except Exception as exc:
    unreal.log_error("[MagicShardImport] " + str(exc))
    sys.exit(1)
