# 魔法碎片：暗蚀纪元 - 3D 技术测试

## 运行方法
*Ctrl* + *Shift* + *`* 新建终端

在终端中执行：
```bash
cd build

#进入第一张地图
./map1.exe

#进入第二章地图
./map2.exe
```

## 操作说明

| 按键 | 功能 |
|------|------|
| W/A/S/D | 前后左右移动 |
| Space | 跳跃 |
| 鼠标拖拽 | 旋转视角 |
| 鼠标滚轮 | 缩放视角 |

## 玩法

控制红色方块在场景中移动，收集场景中漂浮的彩色水晶球。全部 10 个集齐后显示通关提示。

## 项目结构

```
d:/VScode Projects/Game_Development/
├── CMakeLists.txt          # 构建配置（自动下载 raylib）
├── src/
│   └── main.cpp            # 游戏主程序
└── build/
    ├── MagicShard.exe      # 编译产物
    └── ...                 # CMake 临时文件
```
