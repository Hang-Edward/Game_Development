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
.\map1.exe
```

### 核心 C++ 模块

| 类 | 功能 |
|------|------|
| `MagicShard` | 模块入口 |
| `MagicShardPlayerCharacter` | 玩家角色（输入、移动、物理） |
| `MagicShardPlayerController` | 玩家控制器（相机、UI 交互） |
| `MagicShardGameMode` | 游戏模式（规则、HUD 创建） |
| `MagicShardBaseCharacter` | 角色基类 |
| `MagicShardCombatComponent` | 战斗组件 |
| `MagicShardHUD` / `MagicShardHUDWidget` | UI/HUD |
| `MagicShardEntity` | 实体系统 |
| `MagicShardInventoryComponent` | 背包 |
| `MagicShardSaveSubsystem` | 存档 |
| `MagicShardShardPickup` | 可拾取碎片 |

### 构建脚本

| 脚本 | 用途 |
|------|------|
| `Scripts/Build-Unreal.ps1` | 编译 C++ 项目 |
| `Scripts/Build-Launcher.ps1` | 生成 `map1.exe` |
| `Scripts/Launch-Editor.ps1` | 启动 UE Editor |
| `Scripts/ImportLegacyAssets.py` | 从 Unity 项目导入 FBX 资产（角色+地图） |
| `Scripts/FixMapCollision.py` | 修复地图 StaticMesh 碰撞（设置 CTF_USE_COMPLEX_AS_SIMPLE） |
| `Scripts/SetupImportedWorld.py` | 一键重建关卡（放置地形+光照+PlayerStart） |
| `Scripts/ReimportMapAndRebuildWorld.py` | 导入+碰撞+场景完整流程 |
| `Scripts/Run-ReimportAndRebuild.ps1` | 上述流程的 PowerShell 封装 |
| `Scripts/Run-RuntimeSmoke.ps1` | 运行时烟雾测试 |

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
- `map_01_forest.glb` (304MB, Git LFS) — 第一章地图
- `character/{stand,walk,run}.glb` — 主角模型（已转为 FBX）
- `boss_spider/` — BOSS1
- `boss3/` — BOSS3

## 版本规则

- 仅递增次版本号 (v0.x) 用于功能性变更。
- 未经明确要求，不得更改主版本号。
- 变更记录在 `log.md`。
