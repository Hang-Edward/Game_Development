# 魔法碎片：暗蚀纪元

3D 第三人称动作冒险项目。当前主线已经迁移到 Unreal Engine，旧的 Unity 原型和 raylib / GLB 资产目录已经从仓库中清理。

## 当前主线

- 引擎：Unreal Engine 5.7
- 语言：C++
- 项目目录：`MagicShard_Unreal/`
- 可执行启动器：`MagicShard_Unreal/map1.exe`
- 主要资产格式：后续统一使用 FBX 导入 Unreal

## 快速开始

```powershell
cd MagicShard_Unreal
.\map1.exe
```

如果需要打开 Unreal Editor：

```powershell
cd MagicShard_Unreal
.\Scripts\Launch-Editor.ps1
```

如果需要重新编译 C++：

```powershell
cd MagicShard_Unreal
.\Scripts\Build-Unreal.ps1
```

## 项目结构

```text
MagicShard_Unreal/
├── Source/MagicShard/          # C++ 游戏模块
│   ├── Public/                 # 头文件
│   └── Private/                # cpp 实现
├── Content/                    # Unreal 资产与关卡
├── Config/                     # Unreal 项目配置
├── Scripts/                    # 构建、启动、验证脚本
├── Launcher/                   # map1.exe 启动器源码
└── MagicShard_Unreal.uproject  # Unreal 项目文件
```

## 常用脚本

| 脚本 | 用途 |
|------|------|
| `Scripts/Build-Unreal.ps1` | 编译 Unreal C++ 项目 |
| `Scripts/Build-Launcher.ps1` | 生成 `map1.exe` 启动器 |
| `Scripts/Launch-Editor.ps1` | 使用项目配置启动 Unreal Editor |
| `Scripts/Run-RuntimeSmoke.ps1` | 运行时烟雾测试 |
| `Scripts/FixMapCollision.py` | 修复地图 StaticMesh 碰撞设置 |
| `Scripts/FixRunAnimation.py` | 修复 Run 动画循环问题 |
| `Scripts/DiagnoseRunAnimation.py` | 诊断 Run 动画帧与骨骼差异 |
| `Scripts/SetupImportedWorld.py` | 基于已导入资产重建测试关卡 |

## 资产约定

旧的 `assets/models/` GLB 目录和 `MagicShard_Unity/` Unity 原型已经删除。后续角色、敌人和地图资产请优先提供 FBX：

- 角色和敌人：网格 + 骨架 + 动画，骨架命名和朝向保持一致。
- 地图：静态网格 FBX，材质贴图单独提供 PNG/TGA 等常规纹理文件。
- 音效：`.wav` 或 `.ogg`。

导入 Unreal 前，优先先用检查脚本确认 FBX 的骨架、网格、动画帧和轴向设置。
