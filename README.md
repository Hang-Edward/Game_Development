# 魔法碎片：暗蚀纪元

Unity 2022.3 LTS 3D 开放世界冒险游戏（C#）。

> 从 raylib + C++17 迁移而来，旧版代码保留在 `src/` 目录作为参考。

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
4. 在 Project 窗口中打开 `Assets/Scenes/GameWorld.unity`
5. 点击 Play 按钮运行

### 3. 复制模型资产

首次运行前，需要将 GLB 模型文件复制到 Unity 项目：

```bash
# 复制角色模型
cp assets/models/character/*.glb MagicShard_Unity/Assets/Models/Character/
# 复制地图模型
cp assets/models/map_*.glb MagicShard_Unity/Assets/Models/Map/
# 复制 BOSS 模型
cp assets/models/boss_spider/*.glb MagicShard_Unity/Assets/Models/Boss_Spider/
cp assets/models/boss3/*.glb MagicShard_Unity/Assets/Models/Boss3/
```

详见 `MagicShard_Unity/Assets/Animations/SETUP_GUIDE.md`。

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
│   ├── Scripts/           # C# 游戏脚本
│   │   ├── PlayerController.cs
│   │   ├── CameraController.cs
│   │   ├── CombatSystem.cs
│   │   ├── UIManager.cs
│   │   ├── GameManager.cs
│   │   └── AnimationStateController.cs
│   ├── Settings/
│   │   └── GameInput.inputactions   # 输入绑定配置
│   ├── Animations/
│   │   ├── Controllers/             # Animator Controller
│   │   └── SETUP_GUIDE.md           # Unity 设置指南
│   ├── Models/          # GLB 模型文件
│   ├── Scenes/          # Unity 场景
│   ├── Prefabs/         # 预制体
│   └── UI/              # UI 资源
├── Packages/
│   └── manifest.json    # 包依赖
└── ProjectSettings/     # Unity 项目设置
```

## 旧版 (raylib C++)

旧版 raylib 代码保留在 `src/` 和 `CMakeLists.txt`，如需编译：

```bash
cd build && cmake .. -G "MinGW Makefiles" && cmake --build . -j4
./map1.exe   # 第一章
./map2.exe   # 第二章
```
