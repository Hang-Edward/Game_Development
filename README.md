# 魔法碎片：暗蚀纪元

3D 开放世界冒险游戏 — Unreal Engine 5.7 (C++)。

## 环境要求

| 工具 | 版本 |
|------|------|
| Unreal Engine | 5.7 |
| Python | 3.x (UE 内置) |
| Git (LFS) | 任意版本 |

## 快速开始

### 克隆仓库

```bash
git lfs install
git clone <仓库地址>
cd Game_Development
```

### 一键构建

```powershell
cd MagicShard_Unreal
.\Scripts\Run-ReimportAndRebuild.ps1   # 导入资产+碰撞+场景
```

### 运行

```powershell
.\map1.exe   # 第一章（银风森林）
```

### 构建（C++ 编译）

```powershell
.\Scripts\Build-Unreal.ps1
```

## 操作说明

| 操作 | 按键 |
|------|------|
| 移动 | WASD |
| 视角 | 鼠标滑动 |
| 缩放 | 滚轮 |
| 跳跃 | Space |
| 冲刺 | Shift |
| 蹲下 | Ctrl |
| 攻击 | 左键 |
| 格挡 | 右键 |
| 释放鼠标 | ESC |

## 项目结构

```
MagicShard_Unreal/
├── Source/MagicShard/          # C++ 游戏模块
│   ├── Public/                 # 头文件
│   └── Private/                # 实现文件
├── Content/                    # UE 资产（.uasset/.umap）
│   └── Imported/               # 导入的 FBX 资产
├── Scripts/                    # 构建/导入脚本
│   ├── Build-Unreal.ps1         # 编译 C++ 项目
│   ├── Build-Launcher.ps1       # 生成 map#.exe
│   ├── ImportLegacyAssets.py    # 导入 Unity 项目的 FBX
│   ├── SetupImportedWorld.py    # 重建关卡场景
│   └── FixMapCollision.py       # 修复地形碰撞
├── Config/                     # 项目配置
└── MagicShard_Unreal.uproject  # UE 项目文件
```

## 资产

原始 GLB 模型位于 `assets/models/`，通过 Blender 转换为 FBX：
- `assets/models/map_01_forest.glb` (304MB, Git LFS)
- `assets/models/character/{stand,walk,run}.glb`
- `assets/models/boss_spider/` (idle, walk, attack)
- `assets/models/boss3/` (static, move, attack1-3, defense, die, stepback)

## 参考项目

`MagicShard_Unity/` — Unity 2022.3 LTS 原型（C#），用于早期验证和资产测试。
