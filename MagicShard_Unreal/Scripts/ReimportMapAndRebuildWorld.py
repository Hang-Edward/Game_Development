"""
重新导入地图资产（启用碰撞几何体）并重建场景。

使用方法：在 UE Editor 中运行此脚本，或通过命令行：
  UnrealEditor.exe "project.uproject" -ExecutePythonScript="path/to/this/script.py" -run=Exits

注意：-run=Exits 让 Editor 在脚本完成后自动退出，
      如果取出该参数则 Editor 保持打开状态方便检查。
"""

import os
import sys

import unreal


ROOT = r"D:\VScode Projects\Game_Development"
MAP_FBX = os.path.join(ROOT, "MagicShard_Unity", "Assets", "Models", "Map", "map_01_forest.fbx")
MAP_DEST = "/Game/Imported/Map"

# 场景设置
MAP_PATH = "/Game/Maps/Map01_Forest"
MAP_MESH_PATH = "/Game/Imported/Map/map_01_forest.map_01_forest"


def log(message):
    unreal.log("[MagicShardRebuild] " + message)


def fail(message):
    unreal.log_error("[MagicShardRebuild] " + message)
    raise RuntimeError(message)


# ============================================================
# 第一步：导入地图（带碰撞几何体）
# ============================================================

def import_map_with_collision():
    if not os.path.exists(MAP_FBX):
        fail("缺少地图文件: " + MAP_FBX)

    log("正在导入地图（启用碰撞几何体）: " + MAP_FBX)

    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", False)

    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", True)
    static_data.set_editor_property("import_uniform_scale", 1.0)
    static_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    # 关键修复：启用碰撞几何体生成
    static_data.set_editor_property("auto_generate_collision", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", MAP_FBX)
    task.set_editor_property("destination_path", MAP_DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tools.import_asset_tasks([task])

    for asset_path in task.get_editor_property("imported_object_paths"):
        log("  导入成功: " + asset_path)

    # 检查地图 mesh 是否已导入并可访问
    mesh = unreal.load_asset(MAP_MESH_PATH)
    if mesh is None:
        fail("地图网格体导入后找不到: " + MAP_MESH_PATH)

    log("地图网格体验证成功: " + mesh.get_path_name())

    # 验证碰撞是否存在
    if hasattr(mesh, "get_collision_trace_flag"):
        log("  碰撞标志: " + str(mesh.get_collision_trace_flag()))
    else:
        log("  (碰撞属性将通过 StaticMeshComponent 的碰撞设置生效)")

    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    return mesh


# ============================================================
# 第二步：重建场景（用新的地图 + 碰撞）
# ============================================================

def rebuild_world(mesh):
    log("正在重建场景: " + MAP_PATH)

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if level_editor is None:
        fail("LevelEditorSubsystem 不可用")

    # 打开或创建关卡
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        log("打开现有关卡: " + MAP_PATH)
        if not level_editor.load_level(MAP_PATH):
            fail("无法加载关卡: " + MAP_PATH)
    else:
        log("创建新关卡: " + MAP_PATH)
        if not level_editor.new_level(MAP_PATH):
            fail("无法创建关卡: " + MAP_PATH)

    # 清理旧的 Actor
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actor_subsystem is None:
        fail("EditorActorSubsystem 不可用")

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

    def spawn_actor(actor_class, location, rotation=(0.0, 0.0, 0.0), label=None):
        actor = actor_subsystem.spawn_actor_from_class(
            actor_class,
            unreal.Vector(*location),
            unreal.Rotator(*rotation),
        )
        if actor is None:
            fail("无法生成 Actor: " + str(actor_class))
        if label:
            actor.set_actor_label(label)
        return actor

    # 放置地图
    map_actor = spawn_actor(
        unreal.StaticMeshActor,
        (0.0, 0.0, 0.0),
        label="Imported_Map01_Forest",
    )
    component = map_actor.get_component_by_class(unreal.StaticMeshComponent)
    if component is None:
        fail("地图 StaticMeshActor 没有 StaticMeshComponent")
    component.set_static_mesh(mesh)
    component.set_collision_profile_name("BlockAll")
    component.set_editor_property("bhidden_in_game", False)
    log("  地图放置完成，碰撞配置: BlockAll")

    # 放置 PlayerStart（角色出生点，提高 Z 确保在地形表面之上）
    # 新地图的地形高度可能和旧地图不同，先给 2000 让角色从空中落下着地
    spawn_actor(unreal.PlayerStart, (0.0, -450.0, 2000.0), label="PlayerStart")

    # 光照
    sun = spawn_actor(
        unreal.DirectionalLight,
        (-300.0, -400.0, 600.0),
        (-45.0, -35.0, 0.0),
        "Sun",
    )
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_comp is not None:
        sun_comp.set_editor_property("intensity", 5.0)

    skylight = spawn_actor(unreal.SkyLight, (0.0, 0.0, 500.0), label="SkyLight")
    sky_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
    if sky_comp is not None:
        sky_comp.set_editor_property("intensity", 1.5)

    spawn_actor(unreal.SkyAtmosphere, (0.0, 0.0, 0.0), label="SkyAtmosphere")

    fog = spawn_actor(unreal.ExponentialHeightFog, (0.0, 0.0, 0.0), label="HeightFog")
    fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_comp is not None:
        fog_comp.set_editor_property("fog_density", 0.01)
        fog_comp.set_editor_property("fog_height_falloff", 0.15)

    # 保存关卡
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        fail("保存关卡失败")
    log("场景保存成功")

    # 额外保存地图资产
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)


# ============================================================
# 主流程
# ============================================================

def main():
    log("=== 开始重新导入地图并重建场景 ===")

    # 先刷新资产注册表，确保干净的导入
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    if registry is None:
        log("警告: AssetRegistry 不可用")

    # 第一步：导入地图
    mesh = import_map_with_collision()

    # 刷新资产注册表
    if registry is not None:
        unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Imported/Map"])

    # 第二步：重建场景
    rebuild_world(mesh)

    # 保存所有资产
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.EditorAssetLibrary.save_directory("/Game/Maps", only_if_is_dirty=True, recursive=True)

    log("=== 全部完成！===")
    log("地图已重新导入（auto_generate_collision = True）")
    log("场景 Map01_Forest 已重建，碰撞配置文件: BlockAll")
    log("你现在可以运行 .\map1.exe 测试角色是否不再坠落")


try:
    main()
except Exception as exc:
    unreal.log_error("[MagicShardRebuild] " + str(exc))
    sys.exit(1)
