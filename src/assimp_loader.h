#ifndef ASSIMP_LOADER_H
#define ASSIMP_LOADER_H

#include "raylib.h"

// 用 Assimp 加载 GLB 模型，绕过 raylib 自带的 glTF 加载器限制
// 支持：u32 索引、多骨架、动画 scale 通道
Model LoadModelAssimp(const char *fileName);
ModelAnimation *LoadModelAnimationsAssimp(const char *fileName, int *animCount);

// 修复动画关键帧：将没有位置数据的骨骼设为绑定位姿位置
// 部分 GLB 导出只有旋转动画，导致所有骨骼模型空间位置为 (0,0,0)
void FixAnimationPose(const Model &model, ModelAnimation &anim);

#endif
