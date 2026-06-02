# CLAUDE.md

This file provides guidance when working with this repository.

## 项目概览

**魔法碎片：暗蚀纪元** 是一个 Unreal Engine 5.7 C++ 第三人称动作冒险项目。

- 当前主线：`MagicShard_Unreal/`
- 旧 Unity 原型：已删除，不再作为资产来源
- 旧 raylib / GLB 资产：已删除，后续改用新的 FBX 资产
- 默认回答语言：中文；代码、命令、路径和错误信息保持原文

## 构建与运行

```powershell
cd MagicShard_Unreal

# 编译 C++ 项目
.\Scripts\Build-Unreal.ps1

# 打开 Unreal Editor
.\Scripts\Launch-Editor.ps1

# 启动游戏入口
.\map1.exe
```

## Unreal 自动化注意事项

这台机器上的 Unreal Editor 启动应优先通过项目 `.uproject` 和脚本入口执行，避免裸启动项目浏览器导致 D3D12 / NPU 相关崩溃。

推荐使用：

```powershell
cd MagicShard_Unreal
.\Scripts\Launch-Editor.ps1
```

如果需要执行 Editor Python 自动化，优先沿用项目里已经验证过的脚本机制，不要随意改成 `-NullRHI` 或裸 `UnrealEditor-Cmd.exe`。

## 核心代码

| 类 | 功能 |
|------|------|
| `MagicShard` | Unreal 模块入口 |
| `AMagicShardPlayerCharacter` | 玩家角色、输入、移动、跳跃和动画状态 |
| `AMagicShardPlayerController` | 玩家控制器和相机相关输入 |
| `AMagicShardGameMode` | 游戏模式和默认 Pawn 配置 |
| `AMagicShardBaseCharacter` | 角色基类 |
| `AMagicShardHUD` / `UMagicShardHUDWidget` | HUD 与运行时 UI |
| `UMagicShardAnimInstance` | 动画状态参数与动画蓝图桥接 |

## 当前脚本

| 脚本 | 用途 |
|------|------|
| `Scripts/Build-Unreal.ps1` | 编译 Unreal C++ 项目 |
| `Scripts/Build-Launcher.ps1` | 生成 `map1.exe` |
| `Scripts/Launch-Editor.ps1` | 使用项目配置启动 Unreal Editor |
| `Scripts/Run-RuntimeSmoke.ps1` | 启动运行时烟雾测试 |
| `Scripts/FixMapCollision.py` | 设置地图碰撞 |
| `Scripts/FixRunAnimation.py` | 修复 Run 动画循环 |
| `Scripts/DiagnoseRunAnimation.py` | 诊断 Run 动画 |
| `Scripts/CreatePrototypeTestMap.py` | 创建测试关卡 |
| `Scripts/SetupImportedWorld.py` | 基于已导入资产重建关卡 |

## 资产工作流

后续资产统一面向 Unreal 导入：

- 角色/敌怪：提供 FBX，包含网格、骨架和需要的动画。
- 动画：Idle、Walk、Run、Attack 等可分文件导出，但骨架必须一致。
- 地图：提供静态网格 FBX 和贴图文件。
- 不再依赖 `MagicShard_Unity/` 或根目录 `assets/models/`。

对新 FBX 的排查应先检查：轴向、单位、是否包含网格、是否包含骨架、动画帧范围、骨骼层级、root motion 和首尾帧闭合情况。
