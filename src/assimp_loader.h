#ifndef ASSIMP_LOADER_H
#define ASSIMP_LOADER_H

#include "raylib.h"

// 用 Assimp 加载 GLB 模型，绕过 raylib 自带的 glTF 加载器限制
// 支持：u32 索引、多骨架、动画 scale 通道
Model LoadModelAssimp(const char *fileName);
ModelAnimation *LoadModelAnimationsAssimp(const char *fileName, int *animCount);

#endif
