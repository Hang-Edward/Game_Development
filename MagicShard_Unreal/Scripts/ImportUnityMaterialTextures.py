import os
import re
import sys

import unreal


ROOT = r"D:\VScode Projects\Game_Development"
UNITY_ASSETS = os.path.join(ROOT, "MagicShard_Unity", "Assets")
CHARACTER_MATERIAL_DIR = os.path.join(UNITY_ASSETS, "Models", "Character", "Materials")
MAP_MATERIAL_DIR = os.path.join(UNITY_ASSETS, "Models", "Map", "Materials")

CHARACTER_MATERIAL_DEST = "/Game/Imported/Character"
CHARACTER_TEXTURE_DEST = "/Game/Imported/Character/Textures"
MAP_MATERIAL_DEST = "/Game/Imported/Map"
MAP_TEXTURE_DEST = "/Game/Imported/Map/Textures"
BASE_TEXTURE_PARAMETER = "BaseTexture"


def log(message):
    unreal.log("[MagicShardTextureFix] " + message)


def fail(message):
    unreal.log_error("[MagicShardTextureFix] " + message)
    raise RuntimeError(message)


def sanitize_asset_name(name):
    return re.sub(r"[^A-Za-z0-9_]", "_", name)


def build_guid_index():
    index = {}
    for folder, _, files in os.walk(UNITY_ASSETS):
        for file_name in files:
            if not file_name.endswith(".meta"):
                continue
            meta_path = os.path.join(folder, file_name)
            try:
                text = open(meta_path, "r", encoding="utf-8", errors="ignore").read()
            except OSError:
                continue
            match = re.search(r"^guid:\s*([0-9a-fA-F]+)", text, re.MULTILINE)
            if not match:
                continue
            asset_path = meta_path[:-5]
            if os.path.exists(asset_path):
                index[match.group(1).lower()] = asset_path
    return index


def parse_unity_materials(material_dir):
    mappings = []
    for file_name in os.listdir(material_dir):
        if not file_name.endswith(".mat"):
            continue
        mat_path = os.path.join(material_dir, file_name)
        text = open(mat_path, "r", encoding="utf-8", errors="ignore").read()
        material_section = text.split("Material:", 1)[-1]
        name_match = re.search(r"m_Name:\s*(.+)", material_section)
        guid_match = re.search(r"- _BaseMap:\s*\n\s*m_Texture:\s*\{fileID:\s*2800000,\s*guid:\s*([0-9a-fA-F]+)", text)
        if not name_match or not guid_match:
            continue
        mappings.append((name_match.group(1).strip(), guid_match.group(1).lower()))
    return mappings


def import_texture(source_path, destination_path):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source_path)
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tools.import_asset_tasks([task])
    imported = task.get_editor_property("imported_object_paths")
    if not imported:
        fail("Texture import produced no assets: " + source_path)
    return unreal.load_asset(imported[0])


def enable_material_usage(material):
    usage_enum = getattr(unreal, "MaterialUsage", None)
    set_usage = getattr(unreal.MaterialEditingLibrary, "set_material_usage", None)
    if material is None or usage_enum is None or set_usage is None:
        return

    skeletal_usage = getattr(usage_enum, "MATUSAGE_SKELETAL_MESH", None)
    if skeletal_usage is None:
        log("Skeletal material usage enum is unavailable in this Unreal build.")
        return

    try:
        set_usage(material, skeletal_usage)
    except Exception as exc:
        log("Unable to enable skeletal mesh material usage: " + str(exc))


def make_parent_material(parent_path):
    material = unreal.load_asset(parent_path)
    if material is not None:
        enable_material_usage(material)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        return material

    package_path, asset_name = parent_path.rsplit("/", 1)
    factory = unreal.MaterialFactoryNew()
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(asset_name, package_path, unreal.Material, factory)
    if material is None:
        fail("Failed to create parent material: " + parent_path)

    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -350,
        0,
    )
    sample.set_editor_property("parameter_name", BASE_TEXTURE_PARAMETER)
    unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    material.set_editor_property("two_sided", True)
    enable_material_usage(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    log("Created parent material: " + material.get_path_name())
    return material


def assign_texture_to_material_instance(material_instance, texture, parent_material):
    if material_instance is None:
        return False
    if texture is None:
        return False

    unreal.MaterialEditingLibrary.set_material_instance_parent(material_instance, parent_material)
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(material_instance, BASE_TEXTURE_PARAMETER, texture)
    unreal.MaterialEditingLibrary.update_material_instance(material_instance)
    unreal.EditorAssetLibrary.save_loaded_asset(material_instance)
    return True


def fix_material_set(material_dir, material_dest, texture_dest, parent_material_path):
    guid_index = build_guid_index()
    parent_material = make_parent_material(parent_material_path)
    fixed = 0

    for material_name, texture_guid in parse_unity_materials(material_dir):
        source_texture = guid_index.get(texture_guid)
        if source_texture is None:
            log("Texture GUID not found for " + material_name + ": " + texture_guid)
            continue
        if not source_texture.lower().endswith((".png", ".jpg", ".jpeg", ".tga")):
            alt_png = source_texture + ".png"
            if os.path.exists(alt_png):
                source_texture = alt_png
            else:
                log("Texture source is not an image for " + material_name + ": " + source_texture)
                continue

        texture = import_texture(source_texture, texture_dest)
        material_asset_name = sanitize_asset_name(material_name)
        if material_dest == MAP_MATERIAL_DEST:
            material_asset_name = {
                "Image_0": "default_tex0_001",
                "Image_1": "default_tex1_001",
            }.get(material_asset_name, material_asset_name)
        material = unreal.load_asset(material_dest + "/" + material_asset_name + "." + material_asset_name)
        if material is None:
            log("Unreal material not found for " + material_name + " at " + material_dest + "/" + material_asset_name)
            continue

        if assign_texture_to_material_instance(material, texture, parent_material):
            fixed += 1
            log("Assigned " + texture.get_path_name() + " -> " + material.get_path_name())

    return fixed


def main():
    fixed_character = fix_material_set(
        CHARACTER_MATERIAL_DIR,
        CHARACTER_MATERIAL_DEST,
        CHARACTER_TEXTURE_DEST,
        CHARACTER_MATERIAL_DEST + "/M_UnityTextureParent",
    )
    fixed_map = fix_material_set(
        MAP_MATERIAL_DIR,
        MAP_MATERIAL_DEST,
        MAP_TEXTURE_DEST,
        MAP_MATERIAL_DEST + "/M_UnityTextureParent",
    )
    unreal.EditorAssetLibrary.save_directory(CHARACTER_TEXTURE_DEST, only_if_is_dirty=False, recursive=True)
    unreal.EditorAssetLibrary.save_directory(MAP_TEXTURE_DEST, only_if_is_dirty=False, recursive=True)
    log("Fixed character materials: " + str(fixed_character))
    log("Fixed map materials: " + str(fixed_map))


try:
    main()
except Exception as exc:
    unreal.log_error("[MagicShardTextureFix] " + str(exc))
    sys.exit(1)
