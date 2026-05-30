import unreal

anim = unreal.load_asset("/Game/Imported/Character/run_Anim.run_Anim")
model = anim.data_model_interface
frames = model.get_number_of_frames()
print(f"Frames: {frames}")

# 获取第一个骨骼轨道看看有什么方法
track = model.get_bone_track_by_index(0)
print(f"Track type: {type(track)}")

# Check RawAnimSequenceTrack
bone_names = model.get_bone_track_names()
print(f"\nTotal bones: {len(bone_names)}, sample: {list(bone_names)[:5]}")

# 对比所有骨骼的首帧和末帧变换
track0 = model.get_bone_track_by_index(0)
print(f"\nRoot track raw data: {track0.internal_track_data}")
print(f"Root track name: {track0.name}")

# 检查 raw data 的可用属性
raw = track0.internal_track_data
for attr in ["pos_keys","rot_keys","scale_keys","PosKeys","RotKeys","ScaleKeys","position","rotation","scale"]:
    try:
        val = getattr(raw, attr)
        print(f"  raw.{attr} = {repr(val)[:200]}")
    except:
        pass
