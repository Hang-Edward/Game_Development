# Unity 项目设置指南

## 前提条件

- Unity 2022.3 LTS+（推荐 2022.3.20f1）
- 以下 Package（manifest.json 已声明，Unity 打开时自动下载）：
  - Input System (1.7+)
  - Cinemachine (2.9+)
  - TextMeshPro (3.0+)
  - URP (14.0+)

## 步骤 1：打开项目

1. 启动 Unity Hub
2. 点击 "Open" → 选择 `MagicShard_Unity/` 目录
3. Unity 会提示 enter safe mode — 选择 "Ignore"（因为缺少 Input System package）
4. 等待 Package Manager 下载所有依赖后，Unity 会自动编译脚本
5. 如果提示 "New Input System" 启用，点击 "Yes"

## 步骤 2：导入模型

1. 从原始项目复制 GLB 文件到 Unity 项目目录：
   ```
   assets/models/map_01_forest.glb → Assets/Models/Map/
   assets/models/map_02.glb → Assets/Models/Map/
   assets/models/character/*.glb → Assets/Models/Character/
   assets/models/boss_spider/*.glb → Assets/Models/Boss_Spider/
   assets/models/boss3/*.glb → Assets/Models/Boss3/
   ```
2. 在 Unity 中，选中导入的模型，在 Inspector 中配置：
   - **角色模型** (stand/walk/run.glb)：
     - Rig 选项卡 → Animation Type: **Humanoid**
     - Avatar Definition: **Create From This Model**
     - 点击 Apply
   - **地图模型** (map_01_forest.glb)：
     - Rig 选项卡 → Animation Type: **None**
     - Mesh 选项卡 → 勾选 Read/Write
     - 点击 Apply
   - **BOSS 模型**：
     - Rig 选项卡 → Animation Type: **Generic** 或 **Humanoid**（视模型而定）

## 步骤 3：提取动画 Clip

1. 在 Project 窗口中选中 `stand.glb`
2. 展开文件，会看到自动导入的动画 clip
3. 选中 clip，在 Inspector 中：
   - 重命名为 `Idle`
   - 设置 Loop Time: ✅
   - 点击 Apply
4. 同样操作 `walk.glb` → 重命名为 `Walk`
5. 同样操作 `run.glb` → 重命名为 `Run`

## 步骤 4：创建 Animator Controller

1. 在 Project 窗口中右键 → Create → Animator Controller
2. 命名为 `CharacterAnimator.controller`
3. 双击打开 Animator 窗口
4. 创建 Blend Tree：
   - 右键 → Create State → From New Blend Tree
   - 命名为 `Locomotion`
   - 双击进入 Blend Tree
   - Inspector 设置：
     - Blend Type: **2D Simple Directional**
     - Parameters: **Speed**
   - 添加 Motion：
     - 将 Idle clip 拖入，Speed 阈值设为 0
     - 将 Walk clip 拖入，Speed 阈值设为 3
     - 将 Run clip 拖入，Speed 阈值设为 6

5. **添加 Parameters**（在 Animator 窗口左下角 Parameters 标签）：
   - `Speed` (Float) — 移动速度
   - `Sprint` (Bool) — 冲刺状态
   - `Crouch` (Bool) — 蹲下状态
   - `Block` (Bool) — 格挡状态
   - `Grounded` (Bool) — 地面状态
   - `Attack` (Trigger) — 攻击触发
   - `MotionSpeed` (Float) — 动画混合速度

6. **攻击动画状态**（可选）：
   - 如果有攻击动画 clip，添加 Attack 状态
   - 创建 Any State → Attack 的 transition
   - 条件设为 Attack trigger
   - Attack → Any State（或 Locomotion）的 transition 设为 exit time

## 步骤 5：创建场景

1. 新建场景：File → New Scene → Standard
2. 保存为 `Assets/Scenes/GameWorld.unity`

### 5.1 设置 URP

如果创建的是 URP 项目：
1. Edit → Project Settings → Graphics
2. 确保 Scriptable Render Pipeline Settings 指向 URP 配置文件
3. 如果没有，在 Assets/Settings/ 创建 URP 配置

### 5.2 创建玩家 GameObject

1. 在 Hierarchy 中右键 → Create Empty → 命名为 `Player`
2. 添加组件：
   - **Character Controller**：
     - Height: 1.8
     - Radius: 0.4
     - Step Offset: 0.3
     - Skin Width: 0.08
   - **Player Input**：
     - Actions: 关联 `Assets/Settings/GameInput.inputactions`
     - Default Map: Gameplay
   - **PlayerController**（我们的脚本）
   - **CameraController**（我们的脚本）
   - **CombatSystem**（我们的脚本）

3. 设置 Tag 为 `Player`

4. 在 Player 下创建子对象：
   - 将角色模型（stand.glb 的 prefab 变体）拖为 Player 的子对象
   - 命名为 `CharacterModel`
   - 确保 Animator 组件存在，Controller 设为 `CharacterAnimator`
   - 在 PlayerController 的 Inspector 中，将 Animator 引用指向此对象

### 5.3 创建相机

如果使用 CameraController 脚本（不依赖 Cinemachine）：
1. 将 CameraController 附加到 Player 对象上
2. CameraController 会自动查找 Main Camera

如果使用 Cinemachine（推荐）：
1. 删除场景中的默认相机
2. Cinemachine → Create FreeLook Camera
3. 设置 Follow 和 Look At 为 Player
4. 调整 Orbit 参数：
   - Top Rig: Radius 6, Height 1.5
   - Middle Rig: Radius 6, Height 0.5  
   - Bottom Rig: Radius 3, Height -0.5

### 5.4 添加地形

1. 将 map_01_forest.glb 拖入场景
2. 选中 → Add Component → **Mesh Collider**（自动生成碰撞体）
3. 位置设为 (0, 0, 0)，缩放 (1, 1, 1)
4. 可选：创建地形材质

### 5.5 创建 UI Canvas

1. 右键 Hierarchy → UI → Canvas
2. Canvas 设置：
   - Render Mode: Screen Space Overlay
   - UI Scale Mode: Scale With Screen Size
   - Reference Resolution: 1280x720
3. 添加子对象：
   - **FPS_Text** (TextMeshPro) — 左上角
   - **AnimInfo_Text** (TextMeshPro) — 左上角第二行
   - **Controls_Hint** (TextMeshPro) — 底部中央
   - **Center_Prompt** (TextMeshPro) — 屏幕中央（点击开始游戏）
   - **Zone_Text** (TextMeshPro) — 屏幕顶部中央（区域名称）
4. 将 Canvas 引用赋给 UIManager 脚本

## 步骤 6：运行测试

1. 点击 Play 按钮
2. 测试 WASD 移动、鼠标视角控制
3. 测试 Shift 冲刺、Ctrl 蹲下、Space 跳跃
4. 测试左键攻击、右键格挡
5. 验证动画切换正常（idle/walk/run smooth blend）

## Blender 重新导出建议

如果希望从 Blender 重新导出带有位置动画的 GLB：

1. 在 Blender 中打开 FBX 源文件
2. 选择骨架 → Pose Mode
3. 选择所有骨骼 → 插入关键帧（位置 + 旋转 + 缩放）
4. 导出为 GLB：
   - File → Export → glTF 2.0 (.glb)
   - 勾选：Selected Objects
   - 勾选：Include → Animations
   - Bake All: ✅
   - 取消勾选：Include → Cameras / Lights
5. 确保在导出前 Apply Scale（Ctrl+A → Scale）
