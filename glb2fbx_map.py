import bpy
import os

# Convert map GLB to FBX
glb_path = "d:/VScode Projects/Game_Development/MagicShard_Unity/Assets/Models/Map/map_01_forest.glb"
if not os.path.exists(glb_path):
    print(f"ERROR: {glb_path} not found")
    exit(1)

fbx_path = glb_path.replace(".glb", ".fbx")
print(f"Converting: {glb_path}")

try:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=glb_path)
    print(f"  Imported: {len(bpy.data.objects)} objects")

    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.fbx(
        filepath=fbx_path,
        check_existing=False,
        use_selection=True,
        mesh_smooth_type='FACE',
        use_mesh_modifiers=True,
        path_mode='COPY',
        embed_textures=True,
    )
    print(f"  Exported OK: {os.path.getsize(fbx_path)} bytes")
except Exception as e:
    print(f"  ERROR: {e}")

print("Done!")
