import bpy
import os
import sys

# Script: Convert GLB to FBX while preserving animation
# Usage: blender --background --python glb2fbx.py

glb_files = [
    "d:/VScode Projects/Game_Development/MagicShard_Unity/Assets/Models/Character/stand.glb",
    "d:/VScode Projects/Game_Development/MagicShard_Unity/Assets/Models/Character/walk.glb",
    "d:/VScode Projects/Game_Development/MagicShard_Unity/Assets/Models/Character/run.glb",
]

for glb_path in glb_files:
    if not os.path.exists(glb_path):
        print(f"SKIP: {glb_path} not found")
        continue

    # Clear scene
    bpy.ops.wm.read_factory_settings(use_empty=True)

    fbx_path = glb_path.replace(".glb", ".fbx")
    print(f"\n=== Converting: {glb_path} → {fbx_path} ===")

    try:
        # Import GLB
        bpy.ops.import_scene.gltf(filepath=glb_path)
        print(f"  Imported: {len(bpy.data.objects)} objects, {len(bpy.data.actions)} actions")

        # Select all and export as FBX with animation
        bpy.ops.object.select_all(action='SELECT')

        bpy.ops.export_scene.fbx(
            filepath=fbx_path,
            check_existing=False,
            use_selection=True,
            bake_anim=True,
            bake_anim_use_all_bones=True,
            bake_anim_simplify_factor=0,
            bake_anim_step=1.0,
            add_leaf_bones=False,
            mesh_smooth_type='FACE',
            use_mesh_modifiers=True,
        )
        print(f"  Exported OK: {os.path.getsize(fbx_path)} bytes")
    except Exception as e:
        print(f"  ERROR: {e}")

print("\n=== All conversions complete ===")
