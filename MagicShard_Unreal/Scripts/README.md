# MagicShard Unreal Scripts

这些脚本只服务当前 Unreal 项目。旧的 Unity 迁移脚本已经删除，后续资产请直接以 FBX 形式导入 Unreal。

## 常用命令

```powershell
cd MagicShard_Unreal

# 编译 C++
.\Scripts\Build-Unreal.ps1

# 启动 Editor
.\Scripts\Launch-Editor.ps1

# 运行烟雾测试
.\Scripts\Run-RuntimeSmoke.ps1

# 重新生成 map1.exe
.\Scripts\Build-Launcher.ps1
```

## 当前脚本

| 脚本 | 用途 |
|------|------|
| `Build-Unreal.ps1` | 调用 Unreal Build Tool 编译项目 |
| `Build-Launcher.ps1` | 编译短启动器 `map1.exe` |
| `Find-UnrealEditor.ps1` | 查找本机 Unreal Editor 路径 |
| `Launch-Editor.ps1` | 通过 `.uproject` 启动 Unreal Editor |
| `Run-RuntimeSmoke.ps1` | 运行游戏循环烟雾测试 |
| `CreatePrototypeTestMap.py` | 创建原型测试地图 |
| `SetupImportedWorld.py` | 使用已导入资产搭建关卡 |
| `FixMapCollision.py` | 设置地图碰撞为复杂碰撞 |
| `FixRunAnimation.py` | 修复 Run 动画循环 |
| `DiagnoseRunAnimation.py` | 分析 Run 动画帧和骨骼差异 |
| `CheckImportOptions.py` | 检查导入选项 |
| `RunPrototypePieSmoke.py` | PIE 烟雾测试脚本 |

## 注意

这台机器上的 Unreal 自动化不要默认使用 `-NullRHI` 或裸 `UnrealEditor-Cmd.exe`。如果需要 Editor Python 自动化，优先使用已经验证过的 `.uproject` 启动流程和项目脚本。
