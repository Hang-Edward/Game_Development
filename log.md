# 开发日志

## 版本规则
- **大版本**（v1.x, v2.x）：仅在开发者显式告知时更新
- **小版本**（v0.1 → v0.2）：每次功能性变更后递增

---

## v0.1 — 游戏框架搭建与角色模型接入

### 项目概览

基于 raylib 6.0 + C++17 的 3D 开放世界冒险游戏，使用 Assimp 库加载 GLB 模型。

### 技术栈

| 组件 | 版本/工具 |
|------|-----------|
| 引擎 | raylib 6.0（自源码编译） |
| 语言 | C++17 |
| 构建系统 | CMake + MinGW-w64 8.1.0 |
| 模型加载 | Assimp 5.4.3（自源码编译） |
| 操作系统 | Windows 11 |
| GPU | NVIDIA RTX 5070 Laptop, OpenGL 3.3 |

### 项目结构

```
d:/VScode Projects/Game_Development/
├── CMakeLists.txt                 # 构建配置（raylib 6.0 + Assimp 5.4.3）
├── src/
│   ├── main.cpp                   # 游戏主程序
│   ├── assimp_loader.h            # Assimp 模型加载器头文件
│   └── assimp_loader.cpp          # Assimp 模型加载器实现
├── assets/
│   ├── models/
│   │   ├── map_01_forest.glb      # 第一章地图（银风森林）
│   │   ├── map_02.glb             # 第二章地图
│   │   ├── character/             # 主角角色模型
│   │   │   ├── stand.glb          # 待机动画
│   │   │   ├── walk.glb           # 行走动画
│   │   │   ├── run.glb            # 冲刺动画
│   │   │   ├── hand.glb           # 手部动作
│   │   │   ├── knee.glb           # 下蹲动作
│   │   │   └── collapse.glb       # 倒地动作
│   │   ├── boss_spider/           # BOSS1：蜘蛛
│   │   │   ├── idle.glb
│   │   │   ├── walk.glb
│   │   │   └── attack.glb
│   │   └── boss3/                 # BOSS3（未命名）
│   │       ├── boss3_static.glb
│   │       ├── boss3_move.glb
│   │       ├── boss3_attack1.glb
│   │       ├── boss3_attack2.glb
│   │       ├── boss3_attack3.glb
│   │       ├── boss3_defense.glb
│   │       ├── boss3_die.glb
│   │       └── boss3_stepback.glb
│   └── maps/ （待规划）
├── build/
│   ├── map1.exe                  # 第一章可执行文件
│   └── map2.exe                  # 第二章可执行文件
├── head_pose_recognition.py       # 头部姿态识别（待接入）
├── gesture_recognition.py         # 手势识别（待接入）
└── 《魔法碎片：暗蚀纪元》游戏剧情大纲.md
```

### 已实现功能

| 功能 | 状态 | 说明 |
|------|------|------|
| 第三人称 3D 相机 | ✅ | 鼠标滑动控制视角（Yaw/Pitch），滚轮缩放 |
| WASD 移动 | ✅ | 相对相机方向前后左右 |
| 跳跃/重力 | ✅ | Space 跳跃，-25 m/s² 重力 |
| 冲刺 | ✅ | Shift 加速到 10 units/s |
| 蹲下 | ✅ | Ctrl 身高减半，速度降至 3 |
| 攻击 | ✅ | 左键攻击动画 + 前向射线检测 |
| 格挡 | ✅ | 右键格挡姿态，减速 |
| GLB 地图加载 | ✅ | 支持多地图（map1/map2 两个可执行文件） |
| 地形碰撞 | ✅ | 从 GLB 网格顶点构建高度场，双线性插值 |
| 自动地图缩放 | ✅ | 小地图自动放大 10x |
| Assimp 模型加载 | ✅ | 替代 raylib 内置 GLB 加载器 |
| 角色动画系统 | ✅ | 待机/行走/冲刺自动切换 |
| 多骨架兼容 | ✅ | 自动剔除 Shadow Catcher 等辅助骨架 |
| 动画缩放通道处理 | ✅ | 剔除 ScaleCompensation 骨骼 |
| 中文渲染 | ✅ | 加载 Windows 黑体（simhei.ttf） |
| 鼠标捕获/释放 | ✅ | ESC 释放，点击捕获 |

### v0.1 改动记录

1. **项目初始化**
   - 搭建 CMake 构建系统，集成 raylib 6.0
   - 创建双目标编译（map1/map2），通过 MAP_FILE 宏区分地图
   - 接入 Assimp 5.4.3（从 Gitee 镜像下载）替代 raylib 内置 GLTF 加载器

2. **编写自定义 Assimp 加载器**（`src/assimp_loader.cpp`）
   - 支持 u32 索引（raylib 原生只支持 u16）
   - 支持多骨架（自动取第一个有效骨架）
   - 将根节点缩放烘焙至骨骼绑定位姿，避免缩放干扰模型位移
   - 正确映射骨骼索引（通过骨骼名称查找全局索引）
   - 加载嵌入式 PNG/JPEG 纹理

3. **角色模型系统**
   - 加载主角模型（stand/walk/run 三套动画）
   - 根据玩家状态自动切换动画
   - 修复因 scale compensation 骨骼导致的模型压扁问题
   - 修复多骨架（skins_count=2）导致的骨骼索引错乱
   - 角色朝向自动跟随移动方向

4. **GLB 地图系统**
   - 从 GLB 网格顶点构建碰撞高度场（400×400 分辨率）
   - 使用网格覆盖面积筛选地形网格（排除岩石/雪/树枝等装饰物）
   - 自动缩放小地图（< 5 units 时放大 10x）
   - 双线性插值采样地形高度

5. **修复的关键问题**
   - u32→u16 索引截断 → 需建模同学在 Blender 中拆分网格至 < 65535 顶点
   - 角色浮空 → 将根节点缩放烘焙至骨骼，消除缩放对位移的影响
   - 角色朝向错误 → 不再使用 model.transform 的旋转，全由骨骼控制
   - 角色"纸片化" → 从动画数据中剔除 scale 通道
   - 地形碰撞偏移 → 构建碰撞网格时将顶点变换到世界空间

6. **已知问题**
   - 第一章地图（map_01_forest.glb）部分网格顶点数 > 65535，触发 raylib 的 u32→u16 截断，导致纹理映射错乱
   - 第二章地图（map_02.glb）同样有少量 u32 索引警告，但影响较小
   - 主角模型有 2 个骨骼系统（角色的 215 骨骼 + Shadow Catcher 的 4 骨骼），已自动忽略第二个

7. **素材整理**
   - 整理 assets/models/ 目录结构
   - 主角、BOSS1（蜘蛛）、BOSS3 模型归档至对应子目录

---

## 使用说明

### 运行
```bash
cd build
./map1   # 第一章
./map2   # 第二章
```

### 操作
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

---

## 日志格式（给 AI 维护者）

每次修改文件后，请在 log.md 中追加记录，格式如下：

```markdown
### v0.x — 标题

#### 改动文件
- `path/to/file` — 改了什么
- `path/to/file` — 改了什么

#### 新增功能/修复
- 功能或修复描述
- 功能或修复描述

#### 注意事项
- 需要注意的事项或已知问题
```

### 版本号规则
- `v0.1` → `v0.2` → `v0.3` ... 仅递增小版本号
- 除非开发者明确要求 `v1.0` / `v2.0` 等大版本号，否则**绝不更改大版本号**
- 每次功能性改动（新增/修改/删除功能）都必须递增小版本号并记录
- 纯文档修改不需要更新版本号，但仍需在日志中记录

### v0.2 - 修复角色局部骨骼扭曲

#### 改动文件
- `src/assimp_loader.cpp` - 将 `*_scaleCompensation` 辅助骨骼折叠到对应真实骨骼，避免手指、腿部、脚部被辅助骨骼和真实骨骼重复拉扯。
- `src/assimp_loader.cpp` - 顶点骨骼权重改为按真实骨骼合并、按权重排序、保留前 4 个影响并归一化。
- `src/char_test.cpp` - 更新过期调试文字。

#### 新增功能/修复
- 修复右手手指、腿、脚等部位在动画播放时出现局部扭曲的问题。
- 角色有效骨骼数从 103 降为 96，移除了参与蒙皮的 ScaleCompensation 辅助节点。

#### 注意事项
- ScaleCompensation 节点仍可存在于 GLB 场景图和动画 channel 中，但不会作为独立蒙皮骨骼使用。

### v0.3 - 临时锁定下肢错误旋转

#### 改动文件
- src/assimp_loader.cpp - 在 FixAnimationPose() 中对 thigh/calf/foot/toe 下肢骨骼使用 bindPose 旋转，避免错误动画旋转导致腿部扭曲。
- src/char_test.cpp - 添加 B 键切换 bind pose/动画姿态，方便之后继续排查动画旋转空间。

#### 新增功能/修复
- 修复 char_test 中腿部、小腿、脚部继续扭曲的问题。

#### 注意事项
- 这是保守视觉修复：下肢动画暂时锁定到绑定姿态，后续恢复 walk/run 腿部动作时需要继续处理 FBX 导出旋转空间。

---

### v0.4 — 底层重构：迁移至 Unity 引擎

#### 背景
角色动画 GLB 文件存在根本性缺陷（所有 position keys 为 (0,0,0)），FixAnimationPose 暴力覆盖方案无法彻底解决骨骼扭曲/重叠问题。C++ raylib + Assimp 的手动 CPU 蒙皮管线调试成本过高，决定迁移到 Unity 引擎利用其成熟的动画系统。

#### 改动文件
- `MagicShard_Unity/` — 全新 Unity 2022.3 LTS 项目
- `MagicShard_Unity/Assets/Scripts/PlayerController.cs` — 玩家移动、物理、输入处理
- `MagicShard_Unity/Assets/Scripts/CameraController.cs` — 第三人称轨道相机
- `MagicShard_Unity/Assets/Scripts/CombatSystem.cs` — 攻击检测与冷却
- `MagicShard_Unity/Assets/Scripts/UIManager.cs` — HUD 和 UI 管理
- `MagicShard_Unity/Assets/Scripts/GameManager.cs` — 游戏初始化与状态管理
- `MagicShard_Unity/Assets/Scripts/AnimationStateController.cs` — 动画参数辅助
- `MagicShard_Unity/Assets/Settings/GameInput.inputactions` — 输入系统绑定定义
- `MagicShard_Unity/Assets/Animations/SETUP_GUIDE.md` — Unity Editor 手动设置步骤
- `MagicShard_Unity/Packages/manifest.json` — 包依赖声明
- `MagicShard_Unity/ProjectSettings/ProjectVersion.txt` — Unity 版本配置
- `MagicShard_Unity/.gitignore` — Unity 项目忽略规则
- `CLAUDE.md` — 重写为 Unity 项目指导
- `README.md` — 更新为 Unity 构建说明
- `.gitignore` — 添加 Unity 项目条目

#### 新增功能/修复
- **引擎迁移**：从 raylib 6.0 + C++17 迁移到 Unity 2022.3 LTS + C#
- **动画系统**：Unity Mecanim Animator + Humanoid Rig 替代手动 CPU 蒙皮
  - Unity 自动处理骨骼映射、GPU 蒙皮、动画混合
  - Animator Blend Tree 实现 idle/walk/run 平滑过渡
- **物理系统**：Unity CharacterController + PhysX 替代手动物理
  - 自动重力、碰撞检测、地面判定
- **相机系统**：Unity 相机 + Cinemachine 替代手动 Camera3D 计算
- **输入系统**：Unity Input System Package 替代 raylib IsKeyDown()
  - 可自定义按键映射
- **UI 系统**：TextMeshPro + Canvas 替代 raylib DrawTextEx()
- **模型导入**：Unity 原生 GLB 导入管线替代 Assimp 自定义加载器
  - Humanoid Avatar 自动解决骨骼名称和层级映射问题

#### 注意事项
- GLB 模型文件需要手动复制到 `MagicShard_Unity/Assets/Models/`（通过 Git LFS 跟踪）
- Unity 项目首次打开需要下载 Package 依赖（自动）
- 旧版 C++ 源码保留在 `src/` 目录作为参考，不再编译
- 设置步骤详见 `MagicShard_Unity/Assets/Animations/SETUP_GUIDE.md`

---

### v0.5 - Unity character movement and animation polish

#### Changed files
- `MagicShard_Unity/Assets/Scripts/PlayerController.cs`
- `MagicShard_Unity/Assets/Editor/CharacterAnimationRepairer.cs`
- `MagicShard_Unity/Assets/Editor/CharacterMaterialPostprocessor.cs`
- `MagicShard_Unity/Assets/Models/Character/stand.fbx.meta`
- `MagicShard_Unity/Assets/Models/Character/walk.fbx.meta`
- `MagicShard_Unity/Assets/Models/Character/run.fbx.meta`

#### Fixes
- Enabled loop pose blending on Idle/Walk/Run imports to smooth animation wraparound.
- Changed visual underground rescue from per-frame foot pinning to whole-model rescue only, avoiding periodic walk/run twitch caused by bounds changes during foot motion.
- Locked jump horizontal velocity at takeoff so sprint jumps carry sprint speed and walk jumps carry walk speed.
