# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

**魔法碎片：暗蚀纪元** — Unity 2022.3 LTS 3D 开放世界冒险游戏。

Unity 项目位于 `MagicShard_Unity/`，GLB 模型文件位于 `assets/models/`。

## 构建与运行

```bash
# 使用 Unity Hub 打开 Unity 项目
# 1. 启动 Unity Hub
# 2. 添加项目 -> MagicShard_Unity/
# 3. 菜单栏 Tools -> MagicShard -> Setup Project (首次/动画更新后运行)
# 4. 菜单栏 Tools -> MagicShard -> Build Scene (一键搭建场景)
# 5. 点击 Play 按钮运行
# 6. 菜单栏 Tools -> MagicShard -> Verify Scene (检查场景配置)
```

## 项目结构

| 目录 | 用途 |
|------|------|
| `Assets/Scripts/` | C# 游戏脚本 |
| `Assets/Editor/` | Editor 工具脚本 |
| `Assets/Animations/Controllers/` | Animator Controller |
| `Assets/Models/Character/` | 角色 FBX 模型 (stand/walk/run) |
| `Assets/Models/Map/` | 地图 FBX 模型 |
| `Assets/Scenes/` | Unity 场景文件 (GameWorld.unity) |
| `Assets/Settings/` | Input System 绑定配置 |
| `Assets/UI/Fonts/` | 字体资产 |
| `Assets/Prefabs/` | 预制体 |
| `assets/models/` | 原始 GLB 模型文件（需复制到 Unity 项目） |

## 核心脚本

| 脚本 | 功能 |
|------|------|
| `PlayerController.cs` | 玩家移动、物理、输入处理。直接读取 `Keyboard.current`/`Mouse.current`。包含地面吸附（SnapToGround）、Coyote Time、跳跃等机制 |
| `CameraController.cs` | 第三人称轨道相机。直接读取鼠标输入，不依赖 PlayerInput 组件 |
| `CombatSystem.cs` | 攻击检测与冷却 |
| `UIManager.cs` | HUD 和 UI 管理 |
| `GameManager.cs` | 游戏初始化 |
| `TerrainCollisionBuilder.cs` | 运行时自动为地形网格添加 MeshCollider。挂载在 Terrain 对象上，OnEnable 时触发 |
| `CharacterMaterialApplier.cs` | 角色材质管理 |
| `SetupHelper.cs` (Editor) | 一键搭建场景 + 材质升级 + 项目配置 |
| `SceneVerifier.cs` (Editor) | 场景完整性检查 |

## 输入系统

所有输入直接在脚本中通过 `Keyboard.current` 和 `Mouse.current` 读取（不使用 PlayerInput 事件模式）。PlayerInput 组件仅用于暴露 InputActionAsset 给编辑器。

| 操作 | 键位 | 代码 |
|------|------|------|
| Move | WASD | `kb.wKey.isPressed` |
| Look | 鼠标移动 | `Mouse.current.delta.ReadValue()` |
| Sprint | Shift | `kb.leftShiftKey.isPressed` |
| Crouch | Ctrl | `kb.leftCtrlKey.isPressed` |
| Jump | Space | `kb.spaceKey.wasPressedThisFrame` |
| Attack | 鼠标左键 | `Mouse.current.leftButton.wasPressedThisFrame` |
| Block | 鼠标右键 | `Mouse.current.rightButton.isPressed` |
| Zoom | 滚轮 | `Mouse.current.scroll.ReadValue()` |
| ToggleCursor | ESC | `kb.escapeKey.wasPressedThisFrame` |

## PlayerController 架构

PlayerController 管理角色移动、物理和输入，核心机制：

- **地面检测**: CharacterController.isGrounded + 球形射线探测（SphereCast）双重验证
- **地面吸附** (`SnapToGround`): 在 Start 时从 Y+5 向下射线检测，将角色吸附到地形表面
- **Coyote Time** (`coyoteTime` = 0.12s): 离开地面后短暂时间内仍可跳跃
- **跳跃忽略** (`jumpGroundIgnoreTime` = 0.12s): 起跳后短暂忽略地面碰撞，防止起跳瞬间被地面拉回
- **模型锁定** (`LateUpdate`): 每帧将动画模型子对象的位置/旋转锁定到初始值，防止根运动干扰
- **视觉补偿** (`KeepVisibleModelAboveControllerFeet`): 当模型脚部穿模到地面以下时自动抬高

## 角色动画管线

```
角色 FBX 文件 (stand/walk/run)
  → Unity ModelImporter (Humanoid Rig)
    → 提取 Animation Clip (Idle/Walk/Run)
      → Animator Controller (CharacterAnimator.controller)
        → Blend Tree: Speed 参数控制 idle/walk/run 混合
          → GPU 蒙皮（Unity 自动处理）
```

Blend Tree 阈值: Idle=0, Walk=3, Run=6。Speed 参数由 PlayerController 根据当前速度设置。

## 地形碰撞系统

- **SetupHelper.CreateTerrain()**: 在场景中实例化地形 FBX，挂载 TerrainCollisionBuilder 组件
- **TerrainCollisionBuilder**: `[ExecuteAlways]` 组件，OnEnable 时遍历所有子 MeshFilter，为每个网格添加 MeshCollider（过滤条件: bounds > 0.01f 以排除极小物体）
- **回退机制**: PlayerController.Start() 中调用 `TerrainCollisionBuilder.EnsureSceneTerrainColliders()` 确保碰撞存在

## Editor 工具 (菜单栏 Tools → MagicShard)

| 菜单项 | 快捷键 | 功能 |
|--------|--------|------|
| Setup Project | Ctrl+Shift+S | 更新 Animator Controller 动画绑定 + 输入/项目设置 |
| Build Scene | Ctrl+Shift+B | 一键重建完整场景（Player + 相机 + 地形 + UI + 材质升级） |
| Verify Scene | - | 检查场景完整性（Player、Camera、Terrain、UI、材质） |

## 材质管线

- GLB → Blender → FBX 导出（带嵌入式纹理）
- Unity 导入后通过 `UpgradeMaterials()` 将 Standard Shader 材质属性映射到 URP/Lit
- 映射表: `_MainTex→_BaseMap`, `_Color→_BaseColor`, `_BumpMap→_BumpMap`, `_Glossiness→_Smoothness`, `_Metallic→_Metallic`

## 资产

原始 GLB 模型位于 `assets/models/`，需复制到 `MagicShard_Unity/Assets/Models/`：
- `assets/models/map_01_forest.glb` (304MB, Git LFS)
- `assets/models/character/stand/walk/run.glb`（已转换为 FBX 放入 Unity 项目）
- `assets/models/boss_spider/` (idle, walk, attack)
- `assets/models/boss3/` (static, move, attack1-3, defense, die, stepback)

## 版本规则

- 仅递增次版本号 (v0.x) 用于功能性变更。
- 未经明确要求，不得更改主版本号。
- 变更记录在 `log.md`。
