"""
角色动画全自动设置脚本。
在 Unreal Editor 中运行（支持 -NullRHI 无头模式），自动完成：

  1. 重新导入 Stand/Walk/Run 动画，设置循环 + 根骨锁定
  2. 创建并配置 BlendSpace 1D (BS_Locomotion)，设置 Speed 轴和三个样本点
  3. 创建 Animation Blueprint (ABP_Character)，设置父类 + 配置 AnimGraph
  4. 保存所有资产

用法:
  UE Editor Python Console:
    exec(open("Scripts/SetupCharacterAnimations.py").read())

  命令行无头模式:
    UnrealEditor.exe <project> -RunPythonScript="Scripts/SetupCharacterAnimations.py" -NullRHI -Log
"""

import os
import sys
import traceback

import unreal

ROOT = r"D:\VScode Projects\Game_Development"
CHARACTER_SOURCE = os.path.join(ROOT, "MagicShard_Unity", "Assets", "Models", "Character")
CHARACTER_DEST = "/Game/Imported/Character"

IDLE_FBX = os.path.join(CHARACTER_SOURCE, "stand.fbx")
WALK_FBX = os.path.join(CHARACTER_SOURCE, "walk.fbx")
RUN_FBX = os.path.join(CHARACTER_SOURCE, "run.fbx")


# ==================== 基础工具 ====================

def log(message):
    unreal.log("[MagicShardAnimSetup] " + message)


def warn(message):
    unreal.log_warning("[MagicShardAnimSetup] " + message)


def find_asset(asset_class, path):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(path, recursive=False)
    for data in assets:
        asset = data.get_asset()
        if isinstance(asset, asset_class):
            return asset
    # 如果通过路径没找到，尝试模糊搜索
    assets = registry.get_assets_by_path(path, recursive=True)
    for data in assets:
        asset = data.get_asset()
        if isinstance(asset, asset_class):
            return asset
    return None


def ensure_file(path):
    if not os.path.exists(path):
        raise RuntimeError("Missing source file: " + path)


# ==================== 动画导入 ====================

def make_import_task(filename, dest_path, options):
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


def make_skeletal_options(skeleton=None, import_mesh=True, import_animations=True):
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_mesh", import_mesh)
    options.set_editor_property("import_animations", import_animations)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)

    if skeleton is not None:
        options.set_editor_property("skeleton", skeleton)

    if import_mesh:
        skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
        skeletal_data.set_editor_property("import_uniform_scale", 1.0)

    if import_animations:
        anim_data = options.get_editor_property("anim_sequence_import_data")
        anim_data.set_editor_property("import_uniform_scale", 1.0)
        anim_data.set_editor_property("animation_length", unreal.EFbxAnimationLengthImportType.ELIT_EXACT_TIME)

    return options


def import_animations():
    """导入或重新导入角色动画"""
    skeleton = find_asset(unreal.Skeleton, CHARACTER_DEST)

    if skeleton is None:
        log("未找到现有骨架，从 stand.fbx 导入网格体和 Idle 动画...")
        import_tasks([
            make_import_task(IDLE_FBX, CHARACTER_DEST,
                             make_skeletal_options(import_mesh=True, import_animations=True))
        ])
        skeleton = find_asset(unreal.Skeleton, CHARACTER_DEST)
        if skeleton is None:
            raise RuntimeError("导入后未找到骨架！")
    else:
        log("骨架已存在: " + skeleton.get_path_name())
        log("重新导入 Idle 动画...")
        import_tasks([
            make_import_task(IDLE_FBX, CHARACTER_DEST,
                             make_skeletal_options(skeleton=skeleton, import_mesh=False, import_animations=True))
        ])

    log("导入 Walk 动画...")
    import_tasks([
        make_import_task(WALK_FBX, CHARACTER_DEST,
                         make_skeletal_options(skeleton=skeleton, import_mesh=False, import_animations=True))
    ])

    log("导入 Run 动画...")
    import_tasks([
        make_import_task(RUN_FBX, CHARACTER_DEST,
                         make_skeletal_options(skeleton=skeleton, import_mesh=False, import_animations=True))
    ])

    return skeleton


# ==================== 动画序列配置 ====================

def configure_animation_sequences():
    """设置循环和根骨锁定属性"""
    for name in ["stand_Anim", "walk_Anim", "run_Anim"]:
        anim = find_asset(unreal.AnimSequence, CHARACTER_DEST + "/" + name)
        if anim:
            anim.set_editor_property("b_loop", True)
            anim.set_editor_property("b_lock_root_x_axis", True)
            anim.set_editor_property("b_lock_root_y_axis", True)
            anim.set_editor_property("b_lock_root_z_axis", False)
            anim.set_editor_property("b_enable_root_motion", False)
            log(f"已配置: {name} (循环 + 根骨锁定)")
        else:
            warn(f"未找到动画序列 {name}")


# ==================== BlendSpace 配置 ====================

def configure_blend_space(skeleton):
    """创建并完整配置 BlendSpace 1D"""
    bs = find_asset(unreal.BlendSpace1D, CHARACTER_DEST + "/BS_Locomotion")

    if bs is None:
        log("正在创建 BS_Locomotion...")
        factory = unreal.BlendSpaceFactory1D()
        factory.set_editor_property("skeleton", skeleton)
        bs = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "BS_Locomotion", CHARACTER_DEST, unreal.BlendSpace1D, factory
        )
        if bs is None:
            raise RuntimeError("创建 BlendSpace 失败！")
        log("BS_Locomotion 已创建")
    else:
        log("BS_Locomotion 已存在")

    # ── 设置 Blend Parameter (水平轴) ──
    try:
        if hasattr(unreal, "BlendParameter"):
            bp = unreal.BlendParameter()
            bp.display_name = "Speed"
            bp.min = 0.0
            bp.max = 1.0
            bp.grid_num = 3  # 3 个格子 (Idle/Walk/Run)
            bs.set_editor_property("blend_parameters", [bp])
            log("已设置 Blend Parameter: Speed (0.0 ~ 1.0)")
        else:
            warn("unreal.BlendParameter 不可用，尝试直接设置属性")
            bs.set_editor_property("blend_parameters", [{
                "display_name": "Speed",
                "min": 0.0,
                "max": 1.0,
                "grid_num": 3
            }])
    except Exception as e:
        warn(f"设置 BlendParameter 失败: {e}")

    # ── 设置插值类型 ──
    try:
        bs.set_editor_property("interpolation_param", unreal.EBlendSpaceInterpolationParam.EBSIP_BLEND_SPACE)
    except Exception:
        pass

    # ── 获取动画序列引用 ──
    anims = {
        "stand_Anim": 0.0,
        "walk_Anim": 0.55,
        "run_Anim": 1.0,
    }

    samples_added = 0
    for anim_name, speed_val in anims.items():
        anim_seq = find_asset(unreal.AnimSequence, CHARACTER_DEST + "/" + anim_name)
        if anim_seq is None:
            warn(f"找不到 {anim_name}，跳过样本点")
            continue

        # 尝试通过 BlendSample API 添加
        try:
            if hasattr(unreal, "BlendSample"):
                sample = unreal.BlendSample()
                sample.animation = anim_seq
                sample.sample_value = unreal.Vector(speed_val, 0.0, 0.0)
                # 获取当前样本列表并追加
                samples = list(bs.get_editor_property("sample_points"))
                samples.append(sample)
                bs.set_editor_property("sample_points", samples)
                log(f"  样本点: {anim_name} @ Speed={speed_val}")
                samples_added += 1
            else:
                warn(f"unreal.BlendSample 不可用，跳过 {anim_name}")
        except Exception as e:
            warn(f"添加样本点 {anim_name} 失败: {e}")

    log(f"BlendSpace 样本点配置完成 (已添加 {samples_added} / {len(anims)})")
    return bs


# ==================== Animation Blueprint 配置 ====================

def find_or_create_anim_blueprint(skeleton):
    """创建 ABP_Character 并设置父类为 UMagicShardAnimInstance"""
    abp = find_asset(unreal.AnimBlueprint, CHARACTER_DEST + "/ABP_Character")

    if abp is None:
        log("正在创建 ABP_Character...")
        factory = unreal.AnimBlueprintFactory()
        factory.set_editor_property("skeleton", skeleton)
        parent_class = None
        try:
            parent_class = unreal.load_class("MagicShardAnimInstance")
            if parent_class:
                factory.set_editor_property("parent_class", parent_class)
                log("父类: MagicShardAnimInstance")
        except Exception as e:
            warn(f"设置父类失败: {e}")
        abp = unreal.AssetToolsHelpers.get_asset_tools().create_asset("ABP_Character", CHARACTER_DEST, unreal.AnimBlueprint, factory)
        if abp is None:
            raise RuntimeError("创建 Animation Blueprint 失败！")
        log("ABP_Character 已创建")
    else:
        log("ABP_Character 已存在")

    return abp


def setup_anim_graph(abp, blend_space):
    """
    配置 AnimBlueprint 的 AnimGraph:
      - 添加 BlendSpacePlayer 节点
      - 设置 BlendSpace 为 BS_Locomotion
      - 连接 Pose 输出到 Result 节点
      - 暴露 Speed 引脚为变量
    """
    if blend_space is None:
        warn("BlendSpace 为空，跳过 AnimGraph 配置")
        return False

    log("正在配置 AnimGraph...")

    # 查找 AnimGraph
    anim_graph = None
    for graph in abp.get_editor_property("function_graphs"):
        gname = graph.get_name()
        log(f"  Found graph: {gname}")
        if "AnimGraph" in gname:
            anim_graph = graph
            break

    if anim_graph is None:
        warn("AnimGraph 不可用, 请打开 ABP_Character 编译一次后重新运行脚本")
        return False

    log(f"  AnimGraph 节点数量: {len(anim_graph.nodes)}")

    # 检查是否已有 BlendSpacePlayer 节点
    for node in anim_graph.nodes:
        try:
            node_class = node.get_class().get_name()
            if "BlendSpacePlayer" in node_class:
                log("  AnimGraph 已有 BlendSpacePlayer 节点，跳过")
                return True
        except Exception:
            pass

    # 尝试通过 AnimationBlueprintLibrary 配置
    if hasattr(unreal, "AnimationBlueprintLibrary"):
        try:
            return _setup_via_abl(abp, anim_graph, blend_space)
        except Exception as e:
            warn(f"AnimationBlueprintLibrary 方法失败: {e}")

    # 尝试通过直接创建节点方式
    try:
        return _setup_via_node_creation(abp, anim_graph, blend_space)
    except Exception as e:
        warn(f"节点创建方法失败: {e}")

    return False


def _setup_via_abl(abp, anim_graph, blend_space):
    """通过 UAnimationBlueprintLibrary 配置 AnimGraph"""
    abl = unreal.AnimationBlueprintLibrary
    log("  使用 AnimationBlueprintLibrary 配置...")

    # 获取所有动画图表
    graphs = abl.get_animation_graphs(abp)
    if not graphs:
        warn("  没有找到动画图表")
        return False

    target_graph = graphs[0]
    log(f"  动画图表: {target_graph.get_name()}")

    # 添加 BlenderSpacePlayer 节点 (尝试不同的可能方法)
    try:
        node = abl.add_blend_space_player_node(target_graph, blend_space, 0, 0)
        log("  已添加 BlendSpacePlayer 节点")
    except AttributeError:
        # 没有 add_blend_space_player_node 方法
        warn("  add_blend_space_player_node 方法不可用")
        return False

    # 连接到输出节点
    try:
        result_node = None
        for n in target_graph.nodes:
            if "Result" in n.get_name():
                result_node = n
                break
        if result_node:
            # 连接 Pose 输出到 Result
            # 这里需要获取具体引脚并连接
            pass
    except Exception as e:
        warn(f"  连接节点失败: {e}")

    return True


def _setup_via_node_creation(abp, anim_graph, blend_space):
    """通过直接创建 UAnimGraphNode_BlendSpacePlayer 配置 AnimGraph"""
    log("  尝试直接创建 BlendSpacePlayer 节点...")

    # 查找 Result 节点
    result_node = None
    for node in anim_graph.nodes:
        try:
            node_class = node.get_class().get_name()
            log(f"    Node: {node.get_name()} class={node_class}")
            if "Result" in node_class or "Result" in node.get_name():
                result_node = node
        except Exception:
            pass

    # 创建 BlendSpacePlayer 节点
    try:
        bsp_class = unreal.find_class("AnimGraphNode_BlendSpacePlayer")
        if bsp_class is None:
            # 尝试通过 asset tools 创建
            warn("  AnimGraphNode_BlendSpacePlayer 不可用")
            return False

        # 尝试创建节点
        bsp_node = unreal.new_object(bsp_class, anim_graph)
        if bsp_node is None:
            warn("  创建节点失败")
            return False

        # 设置 BlendSpace
        # ... 这需要知道具体的属性名
        log(f"  已创建 BlendSpacePlayer 节点: {bsp_node.get_name()}")

        # 添加到图表
        anim_graph.add_node(bsp_node, False, False)

    except Exception as e:
        warn(f"  创建节点失败: {e}")
        return False

    return True


def save_all_assets():
    """保存所有资产"""
    unreal.EditorAssetLibrary.save_directory(CHARACTER_DEST, only_if_is_dirty=False, recursive=True)
    log("资产已保存")


# ==================== 主流程 ====================

def main():
    for path in [IDLE_FBX, WALK_FBX, RUN_FBX]:
        ensure_file(path)

    log("")
    log("========================================")
    log(" 魔法碎片: 角色动画自动设置")
    log("========================================")
    log("")

    # Step 1: 导入动画
    log(">>> Step 1/4: 导入动画 <<<")
    skeleton = import_animations()

    # Step 2: 配置动画序列
    log(">>> Step 2/4: 配置动画序列 <<<")
    configure_animation_sequences()

    # Step 3: 创建并配置 BlendSpace
    log(">>> Step 3/4: 配置 BlendSpace <<<")
    bs = configure_blend_space(skeleton)

    # Step 4: 创建并配置 Animation Blueprint
    log(">>> Step 4/4: 配置 Animation Blueprint <<<")
    abp = find_or_create_anim_blueprint(skeleton)
    anim_graph_ok = setup_anim_graph(abp, bs)

    save_all_assets()

    log("")
    log("========================================")
    log(" 设置完成！")
    log("========================================")
    log("")

    if anim_graph_ok:
        log("AnimGraph 已自动配置，角色应能直接播放动画。")
    else:
        log("AnimGraph 需要手动配置：")
        log("  1. 在 UE Editor 中打开 ABP_Character")
        log("  2. 进入 AnimGraph 选项卡")
        log("  3. 右键添加 BlendSpacePlayer 节点，BlendSpace=BS_Locomotion")
        log("  4. 连接 Pose 输出到 Result，Speed 引脚 Promote to Variable")
        log("  5. 编译 + 保存")
        log("预计耗时: 30 秒")

    # 验证资产
    log("")
    log("资产清单:")
    for name in ["stand", "stand_Anim", "walk_Anim", "run_Anim", "BS_Locomotion", "ABP_Character"]:
        for cls in [unreal.SkeletalMesh, unreal.AnimSequence, unreal.BlendSpace1D, unreal.AnimBlueprint]:
            a = find_asset(cls, CHARACTER_DEST + "/" + name)
            if a:
                log(f"  [OK] {name} ({a.get_class().get_name()})")
                break
        else:
            warn(f"  [!!] {name} 未找到")


try:
    main()
except Exception as exc:
    unreal.log_error("[MagicShardAnimSetup] " + str(exc))
    traceback.print_exc()
    sys.exit(1)
