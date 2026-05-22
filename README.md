# 魔法碎片：暗蚀纪元

Unity 2022.3 LTS 3D 开放世界冒险游戏（C#）。

## 环境要求

| 工具 | 版本要求 |
|------|---------|
| Unity | 2022.3 LTS+ |
| Git | 任意版本 |

## 构建与运行

### 1. 克隆仓库

```bash
git clone https://github.com/你的用户名/项目名.git
cd 项目目录
```

### 2. 打开 Unity 项目

1. 启动 Unity Hub
2. 点击 "添加" → 选择 `MagicShard_Unity/` 目录
3. Unity 会自动下载依赖 Package（Cinemachine、Input System、TextMeshPro 等）

### 3. 一键搭建场景

菜单栏 → **Tools → MagicShard → Setup Project**（配置动画 + 项目设置）

菜单栏 → **Tools → MagicShard → Build Scene**（自动搭建完整场景）

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

## Unity 项目结构

```
MagicShard_Unity/
├── Assets/
│   ├── Scripts/                    # C# 游戏脚本
│   │   ├── PlayerController.cs     # 移动/物理/输入
│   │   ├── CameraController.cs     # 第三人称轨道相机
│   │   ├── CombatSystem.cs         # 攻击系统
│   │   ├── UIManager.cs            # HUD
│   │   ├── GameManager.cs          # 游戏初始化
│   │   ├── TerrainCollisionBuilder.cs  # 地形碰撞
│   │   └── CharacterMaterialApplier.cs # 角色材质
│   ├── Editor/                     # Editor 工具
│   │   ├── SetupHelper.cs          # 一键搭建场景
│   │   └── SceneVerifier.cs        # 场景检查
│   ├── Settings/
│   │   └── GameInput.inputactions  # 输入绑定
│   ├── Animations/Controllers/     # Animator Controller
│   ├── Models/                     # FBX 模型文件
│   ├── Scenes/                     # Unity 场景
│   └── UI/Fonts/                   # 字体
├── Packages/manifest.json          # 包依赖
└── ProjectSettings/                # Unity 项目设置
```

## 资产

原始 GLB 模型位于 `assets/models/`，用 Blender 转换为 FBX 后放入 Unity 项目：
- `assets/models/map_01_forest.glb` (304MB, Git LFS)
- `assets/models/character/` (stand.walk.run.glb → .fbx)
- `assets/models/boss_spider/` (idle, walk, attack)
- `assets/models/boss3/` (static, move, attack1-3, defense, die, stepback)
