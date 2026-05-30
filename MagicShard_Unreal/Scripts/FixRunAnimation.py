import unreal

anim = unreal.load_asset("/Game/Imported/Character/run_Anim.run_Anim")
ctrl = anim.controller
model = anim.data_model_interface

old_frames = model.get_number_of_frames()
old_length = model.get_play_length()
print(f"Before: frames={old_frames}, length={old_length:.4f}s")

# 开始修改
ctrl.open_bracket("Trim run anim last frame")

new_frames = old_frames - 1
new_length = old_length * new_frames / old_frames
ctrl.set_number_of_frames(unreal.FrameNumber(value=new_frames))
ctrl.set_play_length(new_length)

ctrl.close_bracket()

print(f"After:  frames={new_frames}, length={new_length:.4f}s")
unreal.EditorAssetLibrary.save_loaded_asset(anim)
print("Saved! Test the run animation.")
