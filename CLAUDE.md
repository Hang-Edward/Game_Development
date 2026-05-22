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
| `Assets/UI/Fonts/` | 中文字体资产 |
| `Assets/Prefabs/` | 预制体 |
| `assets/models/` | 原始 GLB 模型文件（需复制到 Unity 项目） |

## 核心脚本

| 脚本 | 功能 |
|------|------|
| `PlayerController.cs` | 玩家移动、物理、输入处理。直接读取 `Keyboard.current`/`Mouse.current` |
| `CameraController.cs` | 第三人称轨道相机。直接读取鼠标输入，不依赖 PlayerInput 组件 |
| `CombatSystem.cs` | 攻击检测与冷却 |
| `UIManager.cs` | HUD 和 UI 管理 |
| `GameManager.cs` | 游戏初始化 |
| `SetupHelper.cs` (Editor) | 一键搭建场景 + 更新 Animator Controller + 材质升级 |
| `SceneVerifier.cs` (Editor) | 场景完整性检查 |

## 输入绑定

所有输入直接在脚本中通过 `Keyboard.current` 和 `Mouse.current` 读取（不使用 PlayerInput 事件模式）：

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

## 角色动画管线

```
角色 FBX 文件 (stand/walk/run)
  → Unity ModelImporter (Humanoid Rig)
    → 提取 Animation Clip (Idle/Walk/Run)
      → Animator Controller (CharacterAnimator.controller)
        → Blend Tree: Speed 参数控制 idle/walk/run 混合
          → GPU 蒙皮（Unity 自动处理）
```

## 重建场景命令

编辑 `SetupHelper.cs` 后，运行 **Tools → MagicShard → Build Scene** 会自动：
1. 配置 FBX 导入（开启 Read/Write）
2. 创建 Player + CharacterController + 脚本
3. 更新 CameraController 到主相机
4. 放置地形 + 隐藏地面碰撞层
5. 创建 UI Canvas + TMP 文本
6. 升级材质到 URP/Lit

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
