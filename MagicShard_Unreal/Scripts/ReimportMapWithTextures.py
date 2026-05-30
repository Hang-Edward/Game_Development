"""
重新导入森林地图 FBX，开启纹理导入修复材质显示。
运行完毕后自动退出编辑器。
"""

import os
import sys
import traceback

import unreal

ROOT = r"D:\VScode Projects\Game_Development"
MAP_SOURCE = os.path.join(ROOT, "MagicShard_Unity", "Assets", "Models", "Map")
MAP_DEST = "/Game/Imported/Map"
MAP_FBX = os.path.join(MAP_SOURCE, "map_01_forest.fbx")


def log(msg):
    unreal.log("[MagicShardMapFix] " + msg)


def ensure_file(path):
    if not os.path.exists(path):
        raise RuntimeError("Missing source file: " + path)


def make_task(filename, dest_path, options):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    return task


def import_tasks(tasks):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tools.import_asset_tasks(tasks)
    for task in tasks:
        log("Imported: " + task.get_editor_property("filename"))
        for asset_path in task.get_editor_property("imported_object_paths"):
            log("  -> " + asset_path)


def make_options():
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)

    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", True)
    static_data.set_editor_property("import_uniform_scale", 1.0)
    static_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    static_data.set_editor_property("auto_generate_collision", True)
    return options


def main():
    ensure_file(MAP_FBX)
    log("Reimporting map with textures enabled...")
    import_tasks([make_task(MAP_FBX, MAP_DEST, make_options())])
    unreal.EditorAssetLibrary.save_directory(MAP_DEST, only_if_is_dirty=False, recursive=True)
    log("Done! Map reimported with textures.")


try:
    main()
except Exception as exc:
    unreal.log_error("[MagicShardMapFix] " + str(exc))
    traceback.print_exc()
finally:
    # 在 -RunPythonScript 模式下，脚本结束会自动退出
    pass
