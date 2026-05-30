"""
永久修复地图材质纹理引用。

运行方式：在 UE Editor 中打开项目，Python Console 执行：
  exec(open("Scripts/FixMapTexturesPermanently.py").read())

脚本会：
1. 读取地图网格体的材质槽位
2. 为每个槽位创建永久 MaterialInstanceConstant
3. 将已有纹理赋值给材质
4. 保存所有资产

之后即使重编译 C++，纹理也不会丢失。
"""

import os
import sys
import traceback

import unreal

MAP_MESH_PATH = "/Game/Imported/Map/map_01_forest"
TEXTURES = [
    "/Game/Imported/Map/Textures/Image_0.Image_0",
    "/Game/Imported/Map/Textures/Image_1.Image_1",
]
PARENT_MATERIAL_PATH = "/Game/Imported/Map/M_UnityTextureParent.M_UnityTextureParent"
OUTPUT_DIR = "/Game/Imported/Map"


def log(msg):
    unreal.log("[MagicShardPermFix] " + msg)


def load_asset(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError("Failed to load: " + path)
    return asset


def get_texture_parameter_names(material):
    """从材质表达式中提取纹理参数名"""
    param_names = []
    expressions = material.get_editor_property("expressions")
    for expr in expressions:
        if expr.get_class().get_name() == "MaterialExpressionTextureSampleParameter":
            try:
                name = expr.get_editor_property("parameter_name")
                param_names.append(name)
            except Exception:
                pass
    return param_names


def main():
    log("=" * 50)
    log("Permanent Map Texture Fix")
    log("=" * 50)

    # 1. 加载地图网格体
    mesh = load_asset(MAP_MESH_PATH)
    log("Loaded mesh: " + mesh.get_name())

    # 2. 加载纹理
    textures = []
    for tex_path in TEXTURES:
        tex = load_asset(tex_path)
        textures.append(tex)
        log("Loaded texture: " + tex.get_name())

    # 3. 加载父材质
    parent_mat = load_asset(PARENT_MATERIAL_PATH)
    log("Loaded parent material: " + parent_mat.get_name())

    # 4. 获取纹理参数名
    tex_param_names = get_texture_parameter_names(parent_mat)
    log("Found texture parameters: " + str(tex_param_names))

    if not tex_param_names:
        tex_param_names = ["Diffuse"]
        log("No texture params found, trying 'Diffuse'")

    # 5. 获取材质槽位信息
    material_slots = mesh.get_editor_property("material_slots")
    log("Mesh has {} material slots".format(len(material_slots)))

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    for slot_idx, slot in enumerate(material_slots):
        existing_mat = slot.material_interface
        if existing_mat:
            log("Slot {} already has material: {}".format(slot_idx, existing_mat.get_name()))
            # 尝试直接设置纹理参数
            if isinstance(existing_mat, unreal.MaterialInstanceConstant):
                for param_name in tex_param_names:
                    tex = textures[slot_idx] if slot_idx < len(textures) else None
                    if tex:
                        try:
                            existing_mat.set_editor_property("texture_parameter_values", [])
                            # 通过参数集合来设置
                            texture_params = existing_mat.get_editor_property("texture_parameter_values")
                            # 创建新的纹理参数值
                            # 尝试直接设置
                            existing_mat.set_texture_parameter_value(param_name, tex)
                            log("  Set {} = {} on existing material".format(param_name, tex.get_name()))
                        except Exception as e:
                            log("  Could not set parameter on existing material: " + str(e))
            continue

        # 如果没有材质，创建一个新的
        tex = textures[slot_idx] if slot_idx < len(textures) else None

        # 创建材质实例包名
        mat_name = "MI_Map_Slot_{}".format(slot_idx)
        mat_path = OUTPUT_DIR + "/" + mat_name

        # 检查是否已存在
        existing = unreal.EditorAssetLibrary.does_asset_exist(mat_path + "." + mat_name)
        if existing:
            log("Material {} already exists, loading...".format(mat_name))
            mic = load_asset(mat_path + "." + mat_name)
        else:
            log("Creating new material instance: " + mat_name)
            mic = asset_tools.create_asset(mat_name, OUTPUT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
            if mic is None:
                log("Failed to create " + mat_name)
                continue
            mic.set_editor_property("parent", parent_mat)

        # 设置纹理参数
        if tex:
            for param_name in tex_param_names:
                try:
                    mic.set_texture_parameter_value(param_name, tex)
                    log("  Set {} = {} on {}".format(param_name, tex.get_name(), mat_name))
                except Exception as e:
                    log("  Failed to set texture param {}: {}".format(param_name, str(e)))

            # 保存材质实例
            unreal.EditorAssetLibrary.save_loaded_asset(mic)

        # 将材质实例赋给网格体槽位
        mesh.set_material(slot_idx, mic)

    # 6. 保存网格体
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.EditorAssetLibrary.save_directory(OUTPUT_DIR, only_if_is_dirty=False, recursive=True)

    log("=" * 50)
    log("DONE! Map textures permanently fixed.")
    log("You can now close the editor and rebuild C++ if needed.")
    log("=" * 50)


try:
    main()
except Exception as exc:
    unreal.log_error("[MagicShardPermFix] " + str(exc))
    traceback.print_exc()
