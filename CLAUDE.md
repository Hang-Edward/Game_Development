# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

**魔法碎片：暗蚀纪元** — 3D 开放世界冒险游戏。

- **主要引擎**: Unreal Engine 5.7 (C++) — `MagicShard_Unreal/`
- **参考项目**: Unity 2022.3 LTS (C#) — `MagicShard_Unity/`
- **资产**: GLB 模型文件位于 `assets/models/`

## Unreal Engine 项目

### 构建与运行

```powershell
# 构建
.\Scripts\Build-Unreal.ps1

# 一键重建全部（导入资产 + 碰撞 + 场景）
.\Scripts\Run-ReimportAndRebuild.ps1

# 或单独运行
.\Scripts\ImportLegacyAssets.py    # 导入 FBX 资产
.\Scripts\FixMapCollision.py       # 修复地图碰撞
.\Scripts\SetupImportedWorld.py    # 重建关卡场景

# 运行
.\main.exe
```

### 核心 C++ 模块

| 类 | 功能 |
|------|------|
| `MagicShard` | 模块入口 |
| `MagicShardPlayerCharacter` | 玩家角色（输入、移动、物理） |
| `MagicShardPlayerController` | 玩家控制器（相机、UI 交互） |
| `MagicShardGameMode` | 游戏模式（规则、HUD 创建） |
| `MagicShardBaseCharacter` | 角色基类 |
| `MagicShardHUD` / `MagicShardHUDWidget` | UI/HUD |
| `MagicShardEntity` | 实体系统（HeroEntity 管理 HP/MP/碎片） |
| `MagicShardAnimInstance` | 动画实例（预留，未来 AnimBlueprint 用） |
| `MagicShardShardPickup` | 可拾取碎片 |

### 构建脚本

| 脚本 | 用途 |
|------|------|
| `Scripts/Build-Unreal.ps1` | 编译 C++ 项目 |
| `Scripts/Build-Launcher.ps1` | 生成 `main.exe` |
| `Scripts/Launch-Editor.ps1` | 启动 UE Editor |
| `Scripts/ImportLegacyAssets.py` | 从 Unity 项目导入 FBX 资产（角色+地图） |
| `Scripts/FixMapCollision.py` | 修复地图 StaticMesh 碰撞（设置 CTF_USE_COMPLEX_AS_SIMPLE） |
| `Scripts/SetupImportedWorld.py` | 一键重建关卡（放置地形+光照+PlayerStart） |
| `Scripts/ReimportMapAndRebuildWorld.py` | 导入+碰撞+场景完整流程 |
| `Scripts/Run-ReimportAndRebuild.ps1` | 上述流程的 PowerShell 封装 |
| `Scripts/Run-RuntimeSmoke.ps1` | 运行时烟雾测试 |
| `Scripts/FixMapTexturesPermanently.py` | 在 Editor 中运行，永久修复地图纹理材质 |
| `Scripts/SetupCharacterAnimations.py` | 导入角色动画 + 创建 BlendSpace/AnimBlueprint |
| `Scripts/DiagnoseRunAnimation.py` | 诊断动画帧数、骨骼差异 |
| `Scripts/FixRunAnimation.py` | 修复跑步动画循环抽搐 |

### 地图碰撞设置（重要）

UE5.7 中启用地形碰撞的正确 API：

```python
mesh = unreal.load_asset("/Game/Imported/Map/map_01_forest.map_01_forest")
body_setup = mesh.get_editor_property("body_setup")
body_setup.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
```

**注意**: `complex_collision_as_simple` 不是 StaticMesh 的直属性，在 BodySetup 上操作。

## Unity 项目（参考）

Unity 项目位于 `MagicShard_Unity/`，用于原型验证和资产测试。

### 操作方式

菜单栏 `Tools → MagicShard`：
- `Setup Project` (Ctrl+Shift+S) — 更新动画绑定+项目配置
- `Build Scene` (Ctrl+Shift+B) — 一键搭建场景
- `Verify Scene` — 场景完整性检查

## 资产

原始 GLB 模型位于 `assets/models/`，通过 Blender 转换为 FBX：
- `map_01_forest.glb` — 第一章地图（当前已是海滩版）
- `character/{stand,walk,run}.glb` — 主角模型（已转为 FBX）
- `boss_spider/` — BOSS1
- `boss3/` — BOSS3

Blender 位于 `D:\应用-Applications\blender.exe`，可用于命令行 FBX 修改：
```powershell
"d:\应用-applications\blender.exe" --background --python script.py
```

## UE Editor 脚本执行规范

### 已知限制（重要！）

此 UE5.7 安装的 Editor 命令行模式存在以下问题：
| 方法 | 结果 | 原因 |
|------|------|------|
| `-NullRHI` | 崩溃 | ContentBrowser 插件空指针 + Intel NPU 冲突 |
| `UnrealEditor-Cmd.exe` | 卡死 | 引擎初始化挂起 |
| `-RunPythonScript=path` | 脚本不执行 | 命令行管线不工作 |
| `-ExecCmds=Python ...` | 脚本不执行 | 命令行管线不工作 |

**经过验证的修复**：在 `Config/DefaultEngine.ini` 中添加 `[SystemSettings]\nr.NNE.Disable=1` 可避免 NPU 崩溃。

### 可靠的 Python 脚本执行方式（init_unreal.py 机制）

**这是本 UE5.7 安装上唯一可靠的自动化方式。**

`-RunPythonScript` 和 `-ExecCmds=Python ...` 在此 UE5.7 上不可用——它们在引擎初始化早期阶段处理，此时日志系统和资产注册表都未就绪，导致编辑器静默退出。

`Content/Python/init_unreal.py` 在引擎完全加载后由 PythonScriptPlugin 自动执行，所有 API 都可用。

**执行时序对比：**
```
-RunPythonScript:
  进程启动 → 读取命令行 → 插件加载(执行脚本) → 引擎初始化 → 静默退出
                                 ↑脚本在此执行，时机太早，API不可用

init_unreal.py:
  进程启动 → 加载项目 → 引擎初始化 → Editor就绪 → PythonPlugin执行脚本
                                                      ↑脚本在此执行，一切就绪
```

流程：
1. 将 Python 脚本写入 `Content/Python/init_unreal.py`
2. 通过 `.\main.exe`（或双击 .uproject）正常启动 Editor
3. 脚本会在 Editor 启动过程中自动执行
4. 脚本最后必须**自删除**（重命名为 `.bak`），防止下次启动重复执行

```python
# 脚本模板
import unreal, os
# ... 执行修复逻辑 ...
unreal.log("[MyTag] Done")
os.rename(__file__, __file__.replace(".py", ".bak"))
```

**注意**：如果 `Content/Python/` 目录不存在，需要先创建。每个 Python 执行任务只需创建一次该文件，执行后会自动清理。

**不推荐的方式**（在 UE5.7 此安装上不可用）：
- `-RunPythonScript=path` — 编辑器在初始化早期阶段就静默退出，脚本不执行
- `-ExecCmds=Python ...` — 同上，日志系统都未初始化
- 让用户手动粘贴到 Output Log — 可以实现但不够自动化

### 熔断机制

在尝试自动执行 Editor 脚本时，必须遵守以下规则：

1. **超时熔断**: 使用 `UnrealEditor-Cmd.exe` 或 `-RunPythonScript` 时，设置 120 秒超时。超时后立即终止进程，不重试同一方法。
2. **失败切换**: 一种方法失败后（崩溃/超时/无输出），记录失败原因，**不要重试**。切换到替代方案（如 C++ 运行时修复或指导用户手动操作）。
3. **进程清理**: 每次尝试前必须 `Stop-Process` 所有残留 `UnrealEditor*` 进程，否则 DLL 被锁定导致链接失败。

### 日志监控规则

执行 Python 脚本或 UE Editor 命令时：

1. **检查日志输出**: 脚本必须写入自定义 Log 标签（如 `[MapTexFix]`），以便通过 `grep` 确认脚本是否真的被执行了。无日志 = 脚本未执行。
2. **检查崩溃原因**: Editor 崩溃后，检查：
   - `D:\VScode Projects\Game_Development\Unreal_Engine_crashed_log.txt`（用户收集的崩溃日志）
   - `Saved/Crashes/` 目录下的最新报告
   - `Saved/Logs/MagicShard_Unreal.log` 末尾的退出信息（如 `Engine exit requested (reason: ...)`）
3. **不可直接信任命令行输出**: UE Editor 命令行可能返回 exit code 0 但脚本实际未执行。必须通过日志中的自定义标签验证执行结果。

### 资产修改诊断流程

当需要诊断或修改 UE 资产时，优先顺序：

1. **C++ (PIE 模式)**: 在游戏代码中添加诊断日志，通过 `BeginPlay`/`Tick` 输出。最可靠，因为 PIE 模式总是正常工作的。
2. **Python (手动粘贴)**: 在 Editor 的 Output Log 底部输入框执行。适合一次性操作（导入、修改资产属性）。
3. **Python API 探索流程**: 当不确定正确的 API 名称时：
   - 先用 `dir(object)` 或 `sorted(dir(object))` 列出所有方法/属性
   - 用 `get_editor_property("name")` 检查可用属性
   - 对于 `UAnimSequence`，关键对象为 `anim.controller`（`AnimationDataController`）和 `anim.data_model_interface`（`AnimationDataModel`）
4. **Blender 作为后备**: 对于 FBX 文件级别的修改（如截帧），可以使用 `blender --background --python script.py`。Blender 位于 `D:\应用-Applications\blender.exe`。

### 一键自动化限制

由于 UE Editor 命令行模式不可用，以下操作不可避免需要用户手动介入：
- 在 Output Log 底部框粘贴一行 `exec(open(...)...)` 命令
- 关闭 Editor 窗口（不会自动退出）

**应当提前告知用户需要手动操作，而不是重复尝试失败的自动化方法。**

## 核心模块（当前状态）

| 类 | 功能 | 状态 |
|------|------|------|
| `MagicShard` | 模块入口 | ✅ |
| `MagicShardPlayerCharacter` | 玩家角色（输入、移动、物理、动画） | ✅ |
| `MagicShardPlayerController` | 玩家控制器（输入绑定） | ✅ |
| `MagicShardGameMode` | 游戏模式 + 地图纹理运行时修复 | ✅ |
| `MagicShardBaseCharacter` | 角色基类（移动速度、动作状态） | ✅ |
| `MagicShardHUD` / `MagicShardHUDWidget` | UI/HUD | ✅ |
| `MagicShardEntity` | 实体系统（HeroEntity 管理 HP/MP/碎片） | ✅ |
| `MagicShardAnimInstance` | 动画实例（预留，未来 AnimBlueprint 用） | ✅ |
| `MagicShardShardPickup` | 可拾取碎片 | ✅ |

## 版本规则

- 仅递增次版本号 (v0.x) 用于功能性变更。
- 未经明确要求，不得更改主版本号。
- 变更记录在 `log.md`。
