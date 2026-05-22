import bpy, os

out_dir = "d:/VScode Projects/Game_Development/MagicShard_Unity/Assets/Models/Character"
models = [
    ("stand", "C:/Users/35342/AppData/Local/Temp/stand.glb"),
    ("walk",  "C:/Users/35342/AppData/Local/Temp/walk.glb"),
    ("run",   "C:/Users/35342/AppData/Local/Temp/run.glb"),
]

for name, glb_path in models:
    if not os.path.exists(glb_path):
        print(f"SKIP: {glb_path} not found")
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    print(f"Importing {name}...")
    bpy.ops.import_scene.gltf(filepath=glb_path)
    fbx_path = os.path.join(out_dir, f"{name}.fbx")
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
        path_mode='COPY',
        embed_textures=True,
    )
    sz = os.path.getsize(fbx_path)
    print(f"  {name}.fbx: {sz} bytes")

print("Done!")
