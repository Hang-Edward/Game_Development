# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目状态：迁移到 Unity

项目已从 raylib + Assimp (C++17) 迁移到 **Unity 2022.3 LTS** (C#)。

- 旧版 C++ 代码保留在 `src/` 目录作为参考
- Unity 项目位于 `MagicShard_Unity/`
- GLB 模型文件仍位于 `assets/models/`（通过软链接或复制到 Unity 项目）

## 构建与运行

```bash
# 使用 Unity Hub 打开 Unity 项目
# 1. 启动 Unity Hub
# 2. 添加项目 -> MagicShard_Unity/
# 3. 在 Unity Editor 中打开场景 Assets/Scenes/GameWorld.unity
# 4. 点击 Play 按钮运行
```

## Unity 项目结构

| 目录/文件 | 用途 |
|-----------|------|
| `Assets/Scripts/PlayerController.cs` | 玩家移动、物理、输入处理 |
| `Assets/Scripts/CameraController.cs` | 第三人称轨道相机 |
| `Assets/Scripts/CombatSystem.cs` | 攻击检测与冷却 |
| `Assets/Scripts/UIManager.cs` | HUD 和 UI 管理 |
| `Assets/Scripts/GameManager.cs` | 游戏初始化与状态管理 |
| `Assets/Scripts/AnimationStateController.cs` | 动画参数辅助 |
| `Assets/Settings/GameInput.inputactions` | 输入系统绑定定义 |
| `Assets/Animations/Controllers/` | Animator Controller 存放位置 |
| `Assets/Animations/SETUP_GUIDE.md` | Unity Editor 手动设置步骤 |
| `Assets/Models/` | GLB 模型文件 |
| `Assets/Scenes/` | Unity 场景文件 |
| `Assets/Prefabs/` | 预制体 |

## 输入绑定

| 操作 | 键位 | 对应旧代码 |
|------|------|-----------|
| Move | WASD | IsKeyDown(KEY_W/S/A/D) |
| Look | 鼠标移动 | GetMouseDelta() |
| Sprint | Shift | IsKeyDown(KEY_LEFT_SHIFT) |
| Crouch | Ctrl | IsKeyDown(KEY_LEFT_CONTROL) |
| Jump | Space | IsKeyPressed(KEY_SPACE) |
| Attack | 鼠标左键 | IsMouseButtonPressed(MOUSE_BUTTON_LEFT) |
| Block | 鼠标右键 | IsMouseButtonDown(MOUSE_BUTTON_RIGHT) |
| Zoom | 滚轮 | GetMouseWheelMove() |
| ToggleCursor | Escape | IsKeyPressed(KEY_ESCAPE) |

## 旧版代码参考

旧版 raylib 代码保留在 `src/` 目录，仅供逻辑参考，不再编译：

| 文件 | 功能 | Unity 替代 |
|------|------|-----------|
| `src/main.cpp` | 游戏循环、输入、物理、相机、渲染 | Unity MonoBehaviour 系统 |
| `src/assimp_loader.cpp` | 自定义 GLB 加载/动画 (922行) | Unity 原生 GLB 导入管线 |
| `src/assimp_loader.h` | 公共 API | Unity 自动处理 |
| `src/char_test.cpp` | 角色查看器 | Unity 场景实时预览 |
| `src/glb_check.cpp` | GLB 诊断工具 | Unity Inspector 调试 |

## 关键设计：Unity 角色动画管线

```
角色 GLB 文件 (stand/walk/run)
  → Unity 导入管线 (Humanoid Rig)
    → Avatar 骨骼映射（自动）
      → Animator Controller 管理动画状态
        → Blend Tree: Speed 参数控制 idle/walk/run 混合
          → GPU 蒙皮（Unity 自动处理）
```

**相比旧代码的核心改进：**
- Unity 的 Humanoid Avatar 自动解决骨骼名称和层级映射问题
- Animator Blend Tree 实现平滑过渡（无需手动 lerp）
- GPU 蒙皮替代 CPU 蒙皮（无需手动更新 VBO）
- PhysX 物理引擎替代手动物理（重力、碰撞、刚体）

## 资产

GLB 模型文件位于 `assets/models/`，需要复制到 `MagicShard_Unity/Assets/Models/`：
- `assets/models/map_01_forest.glb` (304MB, Git LFS)
- `assets/models/character/stand.glb`, `walk.glb`, `run.glb`
- `assets/models/boss_spider/` (idle, walk, attack)
- `assets/models/boss3/` (static, move, attack1-3, defense, die, stepback)

## 已知的 GLB 导出问题

旧版 C++ 代码中诊断出的 GLB 问题在 Unity 中可能部分减轻（Humanoid Rig 自动处理），但仍建议重新导出：
- Position keys 全部为 (0,0,0) — Unity Humanoid 会通过 IK 推断位置
- 根节点缩放 0.01 — Unity Import 的 Scale Factor 可校正
- 骨骼跨网格重复 — Unity 自动合并

## 版本规则

- 仅递增次版本号 (v0.x) 用于功能性变更。
- 未经明确要求，不得更改主版本号。
- 变更记录在 `log.md`。
