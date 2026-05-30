"""
地图纹理永久修复。
在 UE Editor 中运行：Python Console 执行一行即可：

  exec(open("Scripts/FixMapTexturesPermanently.py").read())

原理：用 import_textures=True 重新导入地图 FBX，让 UE 自动创建带纹理引用的材质。
"""

import os
import unreal

ROOT = r"D:\VScode Projects\Game_Development"
FBX = os.path.join(ROOT, "MagicShard_Unity", "Assets", "Models", "Map", "map_01_forest.fbx")
DEST = "/Game/Imported/Map"


def log(msg):
    unreal.log("[MapTexFix] " + msg)


# 创建导入选项
options = unreal.FbxImportUI()
options.set_editor_property("automated_import_should_detect_type", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
options.set_editor_property("import_as_skeletal", False)
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_materials", True)
options.set_editor_property("import_textures", True)

data = options.get_editor_property("static_mesh_import_data")
data.set_editor_property("combine_meshes", True)
data.set_editor_property("import_uniform_scale", 1.0)
data.set_editor_property("auto_generate_collision", True)

# 创建导入任务
task = unreal.AssetImportTask()
task.set_editor_property("filename", FBX)
task.set_editor_property("destination_path", DEST)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)

# 执行导入
log("Reimporting map with textures...")
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

for path in task.get_editor_property("imported_object_paths"):
    log("  Imported: " + path)

unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
log("Done! Map textures permanently fixed.")
