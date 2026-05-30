# MagicShard_Unreal — UE5.7 Python 脚本注入规范

## 已知限制：-RunPythonScript 在此 UE5.7 上不可用

| 方式 | 结果 | 原因 |
|------|------|------|
| `-RunPythonScript=path` | ❌ 静默退出 | 执行时机太早，引擎未初始化完 |
| `-ExecCmds=Python ...` | ❌ 静默退出 | 同上 |
| `UnrealEditor-Cmd.exe` | ❌ 卡死 | 命令行模式在此安装有问题 |

## 正确方式：Content/Python/init_unreal.py

UE5 原生支持在引擎完全启动后自动执行 `Content/Python/init_unreal.py`。

### 执行时序对比

```
-RunPythonScript:
  进程启动 → 读取命令行 → 插件加载(执行脚本) → 引擎初始化 → 崩溃/退出
                                ↑脚本在此执行，时机太早，API不可用

init_unreal.py:
  进程启动 → 加载项目 → 引擎初始化 → Editor就绪 → PythonPlugin执行init_unreal.py
                                                      ↑脚本在此执行，一切就绪
```

### 使用流程

1. 将脚本写入 `Content/Python/init_unreal.py`
2. 通过 `.\main.exe`（或双击 .uproject）正常启动 Editor
3. 脚本在 Editor 启动过程中自动执行
4. 脚本最后自删除（重命名为 `.bak`），避免下次重复执行

### 脚本模板

```python
import unreal, os

# 执行修复逻辑
anim = unreal.load_asset("/Game/Imported/Character/run_Anim.run_Anim")
unreal.log("[MyTag] Done")

# 自删除
os.rename(__file__, __file__.replace(".py", ".bak"))
```

### 注意

- 如果 `Content/Python/` 目录不存在，需先创建
- 每个任务只需创建一次该文件，执行后自动清理
- 监控日志中自定义标签（如 `[MyTag]`）确认执行结果
- 执行完毕后 Editor 不会自动退出，需要关闭窗口
