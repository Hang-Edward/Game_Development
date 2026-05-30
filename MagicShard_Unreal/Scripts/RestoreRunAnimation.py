import unreal

skeleton = unreal.load_asset("/Game/Imported/Character/stand_Skeleton.stand_Skeleton")

options = unreal.FbxImportUI()
options.set_editor_property("automated_import_should_detect_type", False)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_mesh", False)
options.set_editor_property("import_animations", True)
options.set_editor_property("import_materials", False)
options.set_editor_property("skeleton", skeleton)

task = unreal.AssetImportTask()
task.set_editor_property("filename", r"D:\VScode Projects\Game_Development\MagicShard_Unity\Assets\Models\Character\run.fbx")
task.set_editor_property("destination_path", "/Game/Imported/Character")
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)

print("Reimporting run.fbx (original frame range)...")
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

anim = unreal.load_asset("/Game/Imported/Character/run_Anim.run_Anim")
sl = anim.get_editor_property("sequence_length")
print(f"Restored: seq_len={sl}s = {sl*24:.0f} frames @24fps")

unreal.EditorAssetLibrary.save_loaded_asset(anim)
print("Done!")
