#ifndef ASSIMP_LOADER_H
#define ASSIMP_LOADER_H

#include "raylib.h"
#include <string>
#include <vector>
#include <map>

// ---------------------------------------------------------------------------
// 原始 Assimp 模型/动画加载（旧接口，保留兼容）
// ---------------------------------------------------------------------------
Model LoadModelAssimp(const char *fileName);
ModelAnimation *LoadModelAnimationsAssimp(const char *fileName, int *animCount);
void FixAnimationPose(const Model &model, ModelAnimation &anim);

// ---------------------------------------------------------------------------
// 基于 Assimp 节点树的完整动画求值系统
// 绕过 raylib 的 UpdateModelAnimation()，直接使用 Assimp scene graph
// ---------------------------------------------------------------------------

// 场景图节点
struct AnimNode {
    std::string name;
    int parent;          // -1 = root
    Transform defaultTransform; // 默认局部变换（从 scene graph 节点提取）
};

// 单节点的动画通道数据
struct AnimChannelData {
    // Position
    std::vector<float> posTimes;
    std::vector<Vector3> posValues;
    // Rotation
    std::vector<float> rotTimes;
    std::vector<Quaternion> rotValues;
    // Scale
    std::vector<float> scaleTimes;
    std::vector<Vector3> scaleValues;
};

// 动画运行时
struct AssimpAnimationRuntime {
    // 节点层次
    std::vector<AnimNode> nodes;
    std::map<std::string, int> nodeNameToIdx;

    // 骨骼映射: boneName → nodeIdx, mOffsetMatrix
    std::vector<int> boneToNode;           // 骨骼索引 → 节点索引
    std::vector<Matrix> boneOffsets;       // mOffsetMatrix（原始值）
    std::vector<std::string> boneNames;
    std::map<std::string, int> boneNameToIdx; // boneName → boneIdx

    // 根节点逆矩阵（用于 GLTF skinning 公式）
    Matrix globalInverseRoot;

    // 每段动画的通道数据
    // animations[a].channels[channelIdx] 对应某个节点的通道
    struct AnimClip {
        std::string name;
        std::vector<int> channelNode;      // 该通道对应的节点索引
        std::vector<AnimChannelData> channels; // 每个通道的数据
        int keyframeCount;                 // 该段动画的总帧数
    };
    std::vector<AnimClip> clips;

    // 工作缓冲区（每帧复用）
    std::vector<Matrix> globalMats;     // 各节点的全局变换
    std::vector<Matrix> skinningMats;   // 骨骼蒙皮矩阵

    bool valid;
};

// 加载动画运行时（从 GLB 文件构建节点树 + 骨骼 + 动画通道）
AssimpAnimationRuntime LoadAssimpAnimationRuntime(const char *fileName);

// 对指定动画段进行求值并更新 model 的顶点缓冲区
// animClipIdx: 0-based 动画段索引
// frame: 当前帧（float，支持帧间插值）
void UpdateModelAnimationAssimp(Model *model, AssimpAnimationRuntime *rt, int animClipIdx, float frame);

// 释放资源
void UnloadAssimpAnimationRuntime(AssimpAnimationRuntime *rt);

#endif
