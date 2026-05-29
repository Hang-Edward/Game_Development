# MagicShard Unreal C++ Rebuild

这是《魔法碎片：暗蚀纪元》的 Unreal Engine C++ 重构版本。

## 课程要求对应关系

- 面向对象核心特性：
  - 类和对象：`UMagicShardEntity`、`AMagicShardPlayerCharacter`、`AMagicShardGameMode` 等。
  - 封装：属性保持 `protected/private`，通过方法读取和修改。
  - 继承：`UMagicShardEntity -> UMagicShardLivingEntity -> UMagicShardHeroEntity`，以及 `ACharacter -> AMagicShardBaseCharacter -> AMagicShardPlayerCharacter`。
  - 多态：`DescribeEntity`、`TickEntity`、`StartPrimaryAction`、`UpdateActionState` 等虚函数。
- 类数量：当前源码中已经超过 10 个自定义类。
- 继承层次：自定义继承至少 3 层。
- UI：`AMagicShardHUD` 和 `UMagicShardHUDWidget`。
- 随机文件处理：
  - `FRandomAccessSaveFile::WriteRecord`
  - `FRandomAccessSaveFile::ReadRecord`
  - `FRandomAccessSaveFile::UpdateRecord`
  - 使用 `std::fstream`、`seekp`、`seekg` 对固定长度记录随机定位。
- 头文件和 cpp 分离：所有主要类均使用 `.h` + `.cpp`。

## 默认操作

- WASD：移动
- Shift：冲刺
- Space：跳跃
- 鼠标：旋转视角
- 鼠标滚轮：连续缩放视角
- 右键：格挡
- F5：写入存档槽 1
- F6：读取存档槽 1
- F7：更新存档槽 1

## 构建

```powershell
cd "D:\VScode Projects\Game_Development\MagicShard_Unreal"
.\Scripts\Build-Unreal.ps1 -UnrealEditorPath "D:\Unreal Engine 5.7\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" -OpenEditor
```

如果 Unreal 安装在常见路径，可以省略 `-UnrealEditorPath`。

## 资产迁移说明

当前源码工程已经准备好接收角色、地图和贴图资产。为了避免再次出现 Unity 阶段的贴图、动画 root 偏移和地面贴合问题，资产迁移建议按以下顺序做：

1. 先导入地图 FBX/GLB，启用碰撞。
2. 再导入角色骨骼和材质。
3. 最后导入 Idle/Walk/Run 动画，并在 Unreal 中用 Animation Blueprint 做状态机。

Unreal 的 `CharacterMovementComponent` 会负责地面交互、上坡下坡、跳跃和空中水平控制，避免手写地面吸附逻辑。
