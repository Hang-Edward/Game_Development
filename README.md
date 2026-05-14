# 魔法碎片：暗蚀纪元

基于 raylib 6.0 + C++17 的 3D 开放世界冒险游戏。

## 环境要求

| 工具 | 版本要求 |
|------|---------|
| CMake | ≥ 3.14 |
| C++ 编译器 | 支持 C++17（GCC 8+, MSVC 2019+） |
| Git | 任意版本 |
| 显卡 | 支持 OpenGL 3.3+ |

## 构建步骤

### 1. 克隆仓库

```bash
git clone https://github.com/你的用户名/项目名.git
cd 项目目录
```

### 2. 配置并编译

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build . -j4
```

> CMake 会自动从 GitHub/Gitee 下载并编译 raylib 6.0 和 Assimp 5.4.3。

### 3. 运行

```bash
# 第一章 银风森林
./map1.exe

# 第二章
./map2.exe
```

> 注意：必须从 `build/` 目录运行，因为资源文件路径是相对于 `build/` 的 `../assets/`。

## 操作说明

| 操作 | 按键 |
|------|------|
| 移动 | WASD |
| 视角 | 鼠标滑动（点击画面后自动锁定） |
| 缩放 | 滚轮 |
| 跳跃 | Space |
| 冲刺 | Shift |
| 蹲下 | Ctrl |
| 攻击 | 左键 |
| 格挡 | 右键 |
| 释放鼠标 | ESC |

## 项目结构

```
├── CMakeLists.txt              # 构建配置
├── src/
│   ├── main.cpp                # 游戏主程序
│   ├── assimp_loader.h         # Assimp 模型加载器
│   └── assimp_loader.cpp
├── assets/
│   └── models/
│       ├── map_01_forest.glb   # 第一章地图
│       ├── map_02.glb          # 第二章地图
│       ├── character/          # 主角模型及动画
│       │   ├── stand.glb
│       │   ├── walk.glb
│       │   └── run.glb
│       ├── boss_spider/        # BOSS1
│       └── boss3/              # BOSS3（未命名）
├── build/                      # 编译输出（已 gitignore）
├── log.md                      # 开发日志
└── .gitignore
```

## 注意事项

- 第一章地图文件较大（~300MB），首次加载需等待数秒
- 如需添加新地图，在 `CMakeLists.txt` 中新增 `add_executable` 并指定 `MAP_FILE` 宏即可
- Python 脚本（`head_pose_recognition.py`）用于头部姿态识别，尚未接入游戏本体
