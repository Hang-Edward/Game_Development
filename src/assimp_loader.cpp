#include "assimp_loader.h"
#include "raymath.h"
#include "rlgl.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>
#include <utility>

// ---------------------------------------------------------------------------
// 工具：Assimp 矩阵 → raylib 矩阵
// ---------------------------------------------------------------------------
static Matrix AiToRl(const aiMatrix4x4 &m)
{
    return {
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    };
}

// ---------------------------------------------------------------------------
// 工具：Assimp 向量 → raylib 向量
// ---------------------------------------------------------------------------
static Vector3 AiToRl(const aiVector3D &v) { return { v.x, v.y, v.z }; }
static Quaternion AiToRl(const aiQuaternion &q) { return { q.x, q.y, q.z, q.w }; }

static bool IsScaleCompensationBone(const std::string &name)
{
    const std::string suffix = "_scaleCompensation";
    return name.size() > suffix.size() &&
           name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string CanonicalBoneName(const std::string &name)
{
    const std::string suffix = "_scaleCompensation";
    if (IsScaleCompensationBone(name)) return name.substr(0, name.size() - suffix.size());
    return name;
}

static bool IsLowerBodyBoneName(const char *name)
{
    if (!name) return false;
    std::string n = name;
    return n.find("_thigh_") != std::string::npos ||
           n.find("_calf_") != std::string::npos ||
           n.find("_foot_") != std::string::npos ||
           n.find("_toe") != std::string::npos;
}

// ---------------------------------------------------------------------------
// 加载纹理（从 GLB 嵌入的数据）
// ---------------------------------------------------------------------------
static Texture2D LoadEmbeddedTexture(const aiTexture *tex)
{
    if (!tex) return { 0 };

    if (tex->mHeight == 0) {
        // 压缩格式 (mWidth = byte size)
        Image img = { 0 };
        if (tex->achFormatHint && strcmp(tex->achFormatHint, "png") == 0)
            img = LoadImageFromMemory(".png", (unsigned char*)tex->pcData, tex->mWidth);
        else if (tex->achFormatHint && strcmp(tex->achFormatHint, "jpg") == 0)
            img = LoadImageFromMemory(".jpg", (unsigned char*)tex->pcData, tex->mWidth);
        if (img.data) {
            Texture2D t = LoadTextureFromImage(img);
            UnloadImage(img);
            return t;
        }
    }
    return { 0 };
}

// ---------------------------------------------------------------------------
// 加载材质
// ---------------------------------------------------------------------------
static void LoadMaterial(aiMaterial *aiMat, Material *rlMat, const aiScene *scene)
{
    *rlMat = LoadMaterialDefault();

    // Base color / diffuse texture
    aiString texPath;
    if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
        const char *str = texPath.C_Str();
        // 嵌入式纹理 (GLB)
        if (str[0] == '*') {
            int idx = atoi(str + 1);
            if (idx >= 0 && idx < (int)scene->mNumTextures) {
                Texture2D t = LoadEmbeddedTexture(scene->mTextures[idx]);
                if (t.id > 0) rlMat->maps[MATERIAL_MAP_DIFFUSE].texture = t;
            }
        }
    }

    // Base color (aiColor4D)
    aiColor4D color(1,1,1,1);
    if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        rlMat->maps[MATERIAL_MAP_DIFFUSE].color = (Color){
            (unsigned char)(color.r * 255),
            (unsigned char)(color.g * 255),
            (unsigned char)(color.b * 255),
            (unsigned char)(color.a * 255)
        };
    }
}

// ---------------------------------------------------------------------------
// 加载 Mesh 顶点数据
// ---------------------------------------------------------------------------
static Mesh aiMeshToRaylib(aiMesh *aiM, const aiScene *scene,
                           std::map<std::string,int> *boneNameToIdx)
{
    Mesh m = { 0 };
    m.vertexCount = aiM->mNumVertices;
    m.triangleCount = aiM->mNumFaces;

    // 顶点位置
    m.vertices = (float *)RL_MALLOC(m.vertexCount * 3 * sizeof(float));
    for (unsigned i = 0; i < aiM->mNumVertices; i++) {
        m.vertices[i*3]   = aiM->mVertices[i].x;
        m.vertices[i*3+1] = aiM->mVertices[i].y;
        m.vertices[i*3+2] = aiM->mVertices[i].z;
    }

    // 法线
    if (aiM->HasNormals()) {
        m.normals = (float *)RL_MALLOC(m.vertexCount * 3 * sizeof(float));
        for (unsigned i = 0; i < aiM->mNumVertices; i++) {
            m.normals[i*3]   = aiM->mNormals[i].x;
            m.normals[i*3+1] = aiM->mNormals[i].y;
            m.normals[i*3+2] = aiM->mNormals[i].z;
        }
    }

    // UV
    if (aiM->HasTextureCoords(0)) {
        m.texcoords = (float *)RL_MALLOC(m.vertexCount * 2 * sizeof(float));
        for (unsigned i = 0; i < aiM->mNumVertices; i++) {
            m.texcoords[i*2]   = aiM->mTextureCoords[0][i].x;
            m.texcoords[i*2+1] = aiM->mTextureCoords[0][i].y;
        }
    }

    // 索引
    int faceIdxCount = 0;
    for (unsigned f = 0; f < aiM->mNumFaces; f++)
        faceIdxCount += aiM->mFaces[f].mNumIndices;

    m.indices = (unsigned short *)RL_MALLOC(faceIdxCount * sizeof(unsigned short));
    int idx = 0;
    for (unsigned f = 0; f < aiM->mNumFaces; f++) {
        aiFace *face = &aiM->mFaces[f];
        for (unsigned j = 0; j < face->mNumIndices; j++)
            m.indices[idx++] = (unsigned short)face->mIndices[j];
    }

    // Bone indices + weights
    if (aiM->HasBones()) {
        const int maxBones = 4;
        m.boneIndices = (unsigned char *)RL_CALLOC(m.vertexCount * 4, sizeof(unsigned char));
        m.boneWeights = (float *)RL_CALLOC(m.vertexCount * 4, sizeof(float));

        std::vector<std::vector<std::pair<int, float>>> influences(m.vertexCount);

        for (unsigned b = 0; b < aiM->mNumBones; b++) {
            aiBone *bone = aiM->mBones[b];
            int boneIdx = -1;
            if (boneNameToIdx) {
                std::string boneName = CanonicalBoneName(bone->mName.C_Str());
                auto it = boneNameToIdx->find(boneName);
                if (it != boneNameToIdx->end()) boneIdx = it->second;
            }
            if (boneIdx < 0) continue;

            for (unsigned w = 0; w < bone->mNumWeights; w++) {
                aiVertexWeight *vw = &bone->mWeights[w];
                unsigned vid = vw->mVertexId;
                if (vid >= (unsigned)m.vertexCount || vw->mWeight <= 0.0f) continue;

                bool merged = false;
                for (auto &inf : influences[vid]) {
                    if (inf.first == boneIdx) {
                        inf.second += vw->mWeight;
                        merged = true;
                        break;
                    }
                }
                if (!merged) influences[vid].push_back({ boneIdx, vw->mWeight });
            }
        }

        for (int vid = 0; vid < m.vertexCount; vid++) {
            auto &inf = influences[vid];
            if (inf.empty()) continue;
            std::sort(inf.begin(), inf.end(), [](const auto &a, const auto &b) {
                return a.second > b.second;
            });

            int influenceCount = ((int)inf.size() < maxBones) ? (int)inf.size() : maxBones;
            float total = 0.0f;
            for (int slot = 0; slot < influenceCount; slot++) total += inf[slot].second;
            if (total <= 0.0f) continue;

            for (int slot = 0; slot < influenceCount; slot++) {
                m.boneIndices[vid * 4 + slot] = (unsigned char)inf[slot].first;
                m.boneWeights[vid * 4 + slot] = inf[slot].second / total;
            }
        }
    }

    if (m.boneIndices) {
        m.animVertices = (float *)RL_MALLOC(m.vertexCount * 3 * sizeof(float));
        memcpy(m.animVertices, m.vertices, m.vertexCount * 3 * sizeof(float));
        if (m.normals) {
            m.animNormals = (float *)RL_MALLOC(m.vertexCount * 3 * sizeof(float));
            memcpy(m.animNormals, m.normals, m.vertexCount * 3 * sizeof(float));
        }
    }
    UploadMesh(&m, false);
    return m;
}

// ---------------------------------------------------------------------------
// 工具：局部→模型空间姿势转换（要求父索引 < 子索引）
// ---------------------------------------------------------------------------
static void ConvertPoseToModelSpace(BoneInfo *bones, int boneCount, Transform *transforms)
{
    // DFS guarantees parent < child, single pass suffices
    for (int i = 0; i < boneCount; i++) {
        int p = bones[i].parent;
        if (p < 0 || p >= i) continue;
        Transform *parentT = &transforms[p];
        Transform *t = &transforms[i];
        t->rotation = QuaternionMultiply(parentT->rotation, t->rotation);
        t->scale = Vector3Multiply(t->scale, parentT->scale);
        t->translation = Vector3Multiply(t->translation, parentT->scale);
        t->translation = Vector3RotateByQuaternion(t->translation, parentT->rotation);
        t->translation = Vector3Add(t->translation, parentT->translation);
    }
}

// ---------------------------------------------------------------------------
// 工具：场景图 DFS 分配骨骼索引（拓扑有序：父 < 子）
// ---------------------------------------------------------------------------
static void AssignBoneIndicesDFS(
    const std::set<std::string> &boneNames,
    const aiScene *scene,
    std::map<std::string, int> &boneNameToIdx,
    std::vector<BoneInfo> &boneInfos)
{
    struct DFSFrame { aiNode *node; int parentBoneIdx; };
    std::vector<DFSFrame> stack;
    stack.push_back({ scene->mRootNode, -1 });
    while (!stack.empty()) {
        DFSFrame f = stack.back(); stack.pop_back();
        std::string name = f.node->mName.C_Str();
        bool isBone = (boneNames.find(name) != boneNames.end());
        int myBoneIdx = -1;
        if (isBone) {
            myBoneIdx = (int)boneInfos.size();
            boneNameToIdx[name] = myBoneIdx;
            BoneInfo bi;
            memset(bi.name, 0, 32);
            strncpy(bi.name, name.c_str(), 31);
            bi.parent = f.parentBoneIdx;
            boneInfos.push_back(bi);
        }
        for (unsigned ci = 0; ci < f.node->mNumChildren; ci++)
            stack.push_back({ f.node->mChildren[ci], (myBoneIdx >= 0) ? myBoneIdx : f.parentBoneIdx });
    }
}

// ===========================================================================
// 加载模型
// ===========================================================================
Model LoadModelAssimp(const char *fileName)
{
    Model model = { 0 };

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(fileName,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        TraceLog(LOG_WARNING, "ASSIMP: Failed to load %s: %s", fileName, importer.GetErrorString());
        return model;
    }

    //TraceLog(LOG_INFO, "ASSIMP: Loaded %s (%d meshes, %d materials, %d animations)", fileName, scene->mNumMeshes, scene->mNumMaterials, scene->mNumAnimations);

    // -- 材质 --
    int matCount = (scene->mNumMaterials > 0) ? (int)scene->mNumMaterials : 1;
    model.materials = (Material *)RL_CALLOC(matCount, sizeof(Material));
    model.materialCount = matCount;
    model.meshMaterial = (int *)RL_CALLOC(scene->mNumMeshes, sizeof(int));

    for (unsigned i = 0; i < scene->mNumMaterials; i++)
        LoadMaterial(scene->mMaterials[i], &model.materials[i], scene);

    if (scene->mNumMaterials == 0)
        model.materials[0] = LoadMaterialDefault();

    // -- 骨骼信息 --
    std::map<std::string, int> boneNameToIdx;
    std::vector<BoneInfo> boneInfos;
    std::vector<Transform> bindPoses;

    // Collect from ALL meshes, sorted by bone count DESC (mesh with most bones first)
    // This ensures the authoritative mOffsetMatrix is used for shared bones
    std::set<std::string> allBoneNames;
    std::map<std::string, aiMatrix4x4> boneOffsets;
    std::vector<int> sortedMeshes;
    for (unsigned i = 0; i < scene->mNumMeshes; i++)
        sortedMeshes.push_back(i);
    std::sort(sortedMeshes.begin(), sortedMeshes.end(), [&](int a, int b) {
        int ca = scene->mMeshes[a]->mNumBones;
        int cb = scene->mMeshes[b]->mNumBones;
        return ca != cb ? ca > cb : a < b;
    });
    for (int mi : sortedMeshes) {
        aiMesh *aiM = scene->mMeshes[mi];
        if (!aiM->HasBones()) continue;
        for (unsigned b = 0; b < aiM->mNumBones; b++) {
            std::string name = aiM->mBones[b]->mName.C_Str();
            if (IsScaleCompensationBone(name)) continue;
            allBoneNames.insert(name);
            if (boneOffsets.find(name) == boneOffsets.end())
                boneOffsets[name] = aiM->mBones[b]->mOffsetMatrix;
        }
    }
    if (!allBoneNames.empty()) {
        AssignBoneIndicesDFS(allBoneNames, scene, boneNameToIdx, boneInfos);
        // extract bind poses
        bindPoses.resize(boneInfos.size());
        for (auto &kv : boneOffsets) {
            auto it = boneNameToIdx.find(kv.first);
            if (it == boneNameToIdx.end()) continue;
            aiMatrix4x4 invBind = kv.second;
            aiMatrix4x4 bind = invBind;
            bind.Inverse();
            aiVector3D pos; aiQuaternion rot; aiVector3D scale;
            bind.Decompose(scale, rot, pos);
            int idx = it->second;
            bindPoses[idx].translation = AiToRl(pos);
            bindPoses[idx].rotation = AiToRl(rot);
            bindPoses[idx].scale = AiToRl(scale);
        }
    }

    // -- 网格 --
    model.meshCount = (int)scene->mNumMeshes;
    model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));

    for (unsigned i = 0; i < scene->mNumMeshes; i++) {
        model.meshes[i] = aiMeshToRaylib(scene->mMeshes[i], scene, &boneNameToIdx);
        model.meshMaterial[i] = (int)scene->mMeshes[i]->mMaterialIndex;
    }

    // -- 根节点变换（标准化去缩放） --
    if (scene->mRootNode) {
        Matrix m = AiToRl(scene->mRootNode->mTransformation);
        Vector3 c0 = Vector3Normalize({m.m0, m.m1, m.m2});
        Vector3 c1 = Vector3Normalize({m.m4, m.m5, m.m6});
        Vector3 c2 = Vector3Normalize({m.m8, m.m9, m.m10});
        float sc = (Vector3Length({m.m0,m.m1,m.m2}) +
                    Vector3Length({m.m4,m.m5,m.m6}) +
                    Vector3Length({m.m8,m.m9,m.m10})) / 3.0f;
        if (fabsf(sc - 1.0f) > 0.001f && !bindPoses.empty()) {
            for (size_t i = 0; i < bindPoses.size(); i++) {
                bindPoses[i].scale.x *= sc;
                bindPoses[i].scale.y *= sc;
                bindPoses[i].scale.z *= sc;
            }
        }
        model.transform = MatrixIdentity();
        model.transform.m0 = c0.x; model.transform.m1 = c0.y; model.transform.m2 = c0.z;
        model.transform.m4 = c1.x; model.transform.m5 = c1.y; model.transform.m6 = c1.z;
        model.transform.m8 = c2.x; model.transform.m9 = c2.y; model.transform.m10 = c2.z;
        model.transform.m12 = m.m12; model.transform.m13 = m.m13; model.transform.m14 = m.m14;
        model.transform.m15 = m.m15;
    }

    // 写入 Model 骨架（bindPose 已是模型空间）
    model.skeleton.boneCount = (int)boneInfos.size();
    if (model.skeleton.boneCount > 0) {
        // 标准化绑定位姿缩放：提取统一缩放因子并移除
        if (!bindPoses.empty()) {
            float uniScale = fabsf(bindPoses[0].scale.x);
            if (uniScale > 0.001f && fabsf(uniScale - 1.0f) > 0.001f) {
                for (size_t i = 0; i < bindPoses.size(); i++) {
                    bindPoses[i].scale.x /= uniScale;
                    bindPoses[i].scale.y /= uniScale;
                    bindPoses[i].scale.z /= uniScale;
                }
            }
        }

        model.skeleton.bones = (BoneInfo *)RL_CALLOC(model.skeleton.boneCount, sizeof(BoneInfo));
        memcpy(model.skeleton.bones, boneInfos.data(), model.skeleton.boneCount * sizeof(BoneInfo));

        //TraceLog(LOG_INFO, "ASSIMP: %s skeleton: %d bones", fileName, model.skeleton.boneCount);

        model.skeleton.bindPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));
        memcpy(model.skeleton.bindPose, bindPoses.data(), model.skeleton.boneCount * sizeof(Transform));

        model.currentPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));
        memcpy(model.currentPose, bindPoses.data(), model.skeleton.boneCount * sizeof(Transform));

        model.boneMatrices = (Matrix *)RL_CALLOC(model.skeleton.boneCount, sizeof(Matrix));
        for (int i = 0; i < model.skeleton.boneCount; i++)
            model.boneMatrices[i] = MatrixIdentity();
    }

    //TraceLog(LOG_INFO, "ASSIMP: Model loaded: %d bones total", model.skeleton.boneCount);
    return model;
}

// ===========================================================================
// 加载动画
// ===========================================================================
ModelAnimation *LoadModelAnimationsAssimp(const char *fileName, int *animCount)
{
    *animCount = 0;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(fileName,
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_LimitBoneWeights | aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode) {
        TraceLog(LOG_WARNING, "ASSIMP: Failed to load anim %s: %s", fileName, importer.GetErrorString());
        return NULL;
    }

    if (scene->mNumAnimations == 0) return NULL;

    // ---- 建立骨骼映射（与 LoadModelAssimp 一致：所有网格 + DFS 拓扑序） ----
    std::map<std::string, int> boneMap;
    std::vector<BoneInfo> boneInfos;

    std::set<std::string> allBoneNames;
    std::vector<int> animSorted;
    for (unsigned i = 0; i < scene->mNumMeshes; i++)
        animSorted.push_back(i);
    std::sort(animSorted.begin(), animSorted.end(), [&](int a, int b) {
        int ca = scene->mMeshes[a]->mNumBones;
        int cb = scene->mMeshes[b]->mNumBones;
        return ca != cb ? ca > cb : a < b;
    });
    for (int mi : animSorted) {
        aiMesh *aiM = scene->mMeshes[mi];
        if (!aiM->HasBones()) continue;
        for (unsigned b = 0; b < aiM->mNumBones; b++) {
            std::string name = aiM->mBones[b]->mName.C_Str();
            if (!IsScaleCompensationBone(name)) allBoneNames.insert(name);
        }
    }
    if (!allBoneNames.empty())
        AssignBoneIndicesDFS(allBoneNames, scene, boneMap, boneInfos);

    // 为每段动画创建一个 ModelAnimation
    *animCount = (int)scene->mNumAnimations;
    ModelAnimation *anims = (ModelAnimation *)RL_CALLOC(*animCount, sizeof(ModelAnimation));

    for (unsigned a = 0; a < scene->mNumAnimations; a++) {
        aiAnimation *aiAnim = scene->mAnimations[a];
        ModelAnimation *rlAnim = &anims[a];

        memset(rlAnim->name, 0, 32);
        strncpy(rlAnim->name, aiAnim->mName.C_Str(), 31);
        rlAnim->boneCount = (int)boneMap.size();

        // compute maxFrames from matched channels only
        int maxFrames = 0;
        for (unsigned c = 0; c < aiAnim->mNumChannels; c++) {
            aiNodeAnim *ch = aiAnim->mChannels[c];
            if (boneMap.find(ch->mNodeName.C_Str()) == boneMap.end()) continue;
            maxFrames = std::max(maxFrames, (int)ch->mNumPositionKeys);
            maxFrames = std::max(maxFrames, (int)ch->mNumRotationKeys);
            maxFrames = std::max(maxFrames, (int)ch->mNumScalingKeys);
        }
        if (maxFrames < 1) maxFrames = 1;

        rlAnim->keyframeCount = maxFrames;
        rlAnim->keyframePoses = (ModelAnimPose *)RL_CALLOC(maxFrames, sizeof(ModelAnimPose));

        // 为每帧分配 Transform 数组（初始化为局部空间单位姿势）
        for (int f = 0; f < maxFrames; f++) {
            rlAnim->keyframePoses[f] = (Transform *)RL_CALLOC(rlAnim->boneCount, sizeof(Transform));
            for (int i = 0; i < rlAnim->boneCount; i++) {
                rlAnim->keyframePoses[f][i].translation = {0,0,0};
                rlAnim->keyframePoses[f][i].rotation = {0,0,0,1};
                rlAnim->keyframePoses[f][i].scale = {1,1,1};
            }
        }

        // 填充每个 channel 的关键帧数据（局部空间）
        for (unsigned c = 0; c < aiAnim->mNumChannels; c++) {
            aiNodeAnim *ch = aiAnim->mChannels[c];
            std::string nodeName = ch->mNodeName.C_Str();
            auto it = boneMap.find(nodeName);
            if (it == boneMap.end()) continue;
            int boneIdx = it->second;

            for (int f = 0; f < maxFrames; f++) {
                Transform *t = &rlAnim->keyframePoses[f][boneIdx];

                if (ch->mNumPositionKeys > 0) {
                    unsigned keyIdx = (ch->mNumPositionKeys == 1) ? 0
                        : (unsigned)((float)f / (maxFrames-1) * (ch->mNumPositionKeys-1) + 0.5f);
                    t->translation = AiToRl(ch->mPositionKeys[keyIdx].mValue);
                }
                if (ch->mNumRotationKeys > 0) {
                    unsigned keyIdx = (ch->mNumRotationKeys == 1) ? 0
                        : (unsigned)((float)f / (maxFrames-1) * (ch->mNumRotationKeys-1) + 0.5f);
                    t->rotation = AiToRl(ch->mRotationKeys[keyIdx].mValue);
                }
                if (ch->mNumScalingKeys > 0) {
                    unsigned keyIdx = (ch->mNumScalingKeys == 1) ? 0
                        : (unsigned)((float)f / (maxFrames-1) * (ch->mNumScalingKeys-1) + 0.5f);
                    t->scale = AiToRl(ch->mScalingKeys[keyIdx].mValue);
                }
            }
        }

        // 修复根骨骼（parent=-1）局部姿势：在模型空间转换前乘以根节点变换
        // 因为根骨骼的局部动画姿势缺少了场景根节点的变换（已烘焙入绑定位姿）
        {
            Matrix rootM = AiToRl(scene->mRootNode->mTransformation);
            aiVector3D rPos2, rScale2; aiQuaternion rRot2;
            scene->mRootNode->mTransformation.Decompose(rScale2, rRot2, rPos2);
            // 去掉根节点的缩放（已烘焙入绑定位姿）
            Vector3 c0 = Vector3Normalize({rootM.m0, rootM.m1, rootM.m2});
            Vector3 c1 = Vector3Normalize({rootM.m4, rootM.m5, rootM.m6});
            Vector3 c2 = Vector3Normalize({rootM.m8, rootM.m9, rootM.m10});
            rootM = MatrixIdentity();
            rootM.m0 = c0.x; rootM.m1 = c0.y; rootM.m2 = c0.z;
            rootM.m4 = c1.x; rootM.m5 = c1.y; rootM.m6 = c1.z;
            rootM.m8 = c2.x; rootM.m9 = c2.y; rootM.m10 = c2.z;
            rootM.m12 = rPos2.x; rootM.m13 = rPos2.y; rootM.m14 = rPos2.z;

            for (int bi = 0; bi < (int)boneInfos.size(); bi++) {
                if (boneInfos[bi].parent >= 0) continue;
                for (int f = 0; f < maxFrames; f++) {
                    Transform *t = &rlAnim->keyframePoses[f][bi];
                    Matrix animM = MatrixMultiply(
                        MatrixScale(t->scale.x, t->scale.y, t->scale.z),
                        QuaternionToMatrix(t->rotation));
                    animM = MatrixMultiply(animM, MatrixTranslate(t->translation.x, t->translation.y, t->translation.z));
                    animM = MatrixMultiply(rootM, animM);
                    t->translation = {animM.m12, animM.m13, animM.m14};
                    t->rotation = QuaternionFromMatrix(animM);
                    t->scale = {1,1,1}; // scale handled by rootM
                }
            }
        }

        // 将每帧从局部空间转换为模型空间（拓扑有序，父索引 < 子索引）
        for (int f = 0; f < maxFrames; f++)
            ConvertPoseToModelSpace(boneInfos.data(), (int)boneInfos.size(), rlAnim->keyframePoses[f]);

        //TraceLog(LOG_INFO, "ASSIMP: Anim '%s' %d frames, %d/%d matched", rlAnim->name, maxFrames, channelsMatched, channelsTotal);
    }

    //TraceLog(LOG_INFO, "ASSIMP: Loaded %d animations (%d bones)", *animCount, (int)boneMap.size());
    return anims;
}

// ---------------------------------------------------------------------------
// 修复动画关键帧：将没有位置数据的骨骼设为绑定位姿位置
// ---------------------------------------------------------------------------
void FixAnimationPose(const Model &model, ModelAnimation &anim)
{
    if (model.skeleton.boneCount == 0 || anim.boneCount == 0) return;
    int boneCount = (model.skeleton.boneCount < anim.boneCount) ? model.skeleton.boneCount : anim.boneCount;
    for (int f = 0; f < anim.keyframeCount; f++) {
        for (int i = 0; i < boneCount; i++) {
            anim.keyframePoses[f][i].translation = model.skeleton.bindPose[i].translation;
            anim.keyframePoses[f][i].scale = model.skeleton.bindPose[i].scale;
        }
    }
}

// ===========================================================================
// 基于 Assimp 节点树的动画求值系统
// ===========================================================================

// 工具：从两帧之间混合插值
static float LerpFloat(float a, float b, float t) { return a + (b - a) * t; }
static Vector3 LerpVec(Vector3 a, Vector3 b, float t) {
    return { LerpFloat(a.x, b.x, t), LerpFloat(a.y, b.y, t), LerpFloat(a.z, b.z, t) };
}

// 检查通道的所有 position 是否都是 (0,0,0)
static bool IsAllZeroPos(const AnimChannelData &ch) {
    for (auto &v : ch.posValues)
        if (fabsf(v.x) > 0.0001f || fabsf(v.y) > 0.0001f || fabsf(v.z) > 0.0001f) return false;
    return ch.posValues.size() > 0;
}

static Matrix InterpolateLocalMatrix(const AnimChannelData &ch, const AnimNode &node, float frame, int maxFrames) {
    // 位置插值
    Vector3 pos = node.defaultTransform.translation;
    if (ch.posValues.size() > 0 && !IsAllZeroPos(ch)) {
        if (ch.posValues.size() == 1) pos = ch.posValues[0];
        else {
            float t = (maxFrames > 1) ? frame / (maxFrames - 1) : 0;
            float kt = t * (ch.posValues.size() - 1);
            int kA = (int)kt; if (kA >= (int)ch.posValues.size()) kA = (int)ch.posValues.size() - 2;
            int kB = kA + 1; if (kB >= (int)ch.posValues.size()) kB = (int)ch.posValues.size() - 1;
            float blend = kt - kA;
            pos = LerpVec(ch.posValues[kA], ch.posValues[kB], blend);
        }
    }

    // 旋转插值
    Quaternion rot = node.defaultTransform.rotation;
    if (ch.rotValues.size() > 0) {
        if (ch.rotValues.size() == 1) rot = ch.rotValues[0];
        else {
            float t = (maxFrames > 1) ? frame / (maxFrames - 1) : 0;
            float kt = t * (ch.rotValues.size() - 1);
            int kA = (int)kt; if (kA >= (int)ch.rotValues.size()) kA = (int)ch.rotValues.size() - 2;
            int kB = kA + 1; if (kB >= (int)ch.rotValues.size()) kB = (int)ch.rotValues.size() - 1;
            float blend = kt - kA;
            rot = QuaternionSlerp(ch.rotValues[kA], ch.rotValues[kB], blend);
        }
    }

    // 缩放插值
    Vector3 sc = node.defaultTransform.scale;
    if (ch.scaleValues.size() > 0) {
        if (ch.scaleValues.size() == 1) sc = ch.scaleValues[0];
        else {
            float t = (maxFrames > 1) ? frame / (maxFrames - 1) : 0;
            float kt = t * (ch.scaleValues.size() - 1);
            int kA = (int)kt; if (kA >= (int)ch.scaleValues.size()) kA = (int)ch.scaleValues.size() - 2;
            int kB = kA + 1; if (kB >= (int)ch.scaleValues.size()) kB = (int)ch.scaleValues.size() - 1;
            float blend = kt - kA;
            sc = LerpVec(ch.scaleValues[kA], ch.scaleValues[kB], blend);
        }
    }

    // raylib row-major: v * S * R * T = scale, then rotate, then translate
    Matrix m = MatrixMultiply(MatrixScale(sc.x, sc.y, sc.z), QuaternionToMatrix(rot));
    m = MatrixMultiply(m, MatrixTranslate(pos.x, pos.y, pos.z));
    return m;
}

// 标准 GLTF 蒙皮公式: jointMatrix = nodeGlobal * inverseBindMatrix
// 在 raylib row-major: v * nodeGlobal * mOffset
static Matrix BuildSkinningMatrix(const Matrix &nodeGlobal, const Matrix &mOffset) {
    return MatrixMultiply(nodeGlobal, mOffset);
}

// ---------------------------------------------------------------------------
// 加载动画运行时
// ---------------------------------------------------------------------------
AssimpAnimationRuntime LoadAssimpAnimationRuntime(const char *fileName) {
    AssimpAnimationRuntime rt;
    rt.valid = false;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(fileName,
        aiProcess_Triangulate | aiProcess_GenNormals |
        aiProcess_LimitBoneWeights | aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode) {
        TraceLog(LOG_WARNING, "ASSIMP_RT: Failed to load %s", fileName);
        return rt;
    }

    // ---- 1. 构建节点树 ----
    struct StackFrame { aiNode *node; int parent; };
    std::vector<StackFrame> stack;
    stack.push_back({scene->mRootNode, -1});
    while (!stack.empty()) {
        StackFrame f = stack.back(); stack.pop_back();
        std::string name = f.node->mName.C_Str();
        if (rt.nodeNameToIdx.find(name) == rt.nodeNameToIdx.end()) {
            int idx = (int)rt.nodes.size();
            rt.nodeNameToIdx[name] = idx;
            AnimNode n;
            n.name = name;
            n.parent = f.parent;
            aiVector3D pos, sc; aiQuaternion rot;
            f.node->mTransformation.Decompose(sc, rot, pos);
            n.defaultTransform.translation = AiToRl(pos);
            n.defaultTransform.rotation = AiToRl(rot);
            n.defaultTransform.scale = AiToRl(sc);
            rt.nodes.push_back(n);
        }
        for (unsigned ci = 0; ci < f.node->mNumChildren; ci++)
            stack.push_back({f.node->mChildren[ci], rt.nodeNameToIdx[name]});
    }

    // ---- 2. 收集骨骼（所有网格）并映射到节点 ----
    for (unsigned i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *aiM = scene->mMeshes[i];
        if (!aiM->HasBones()) continue;
        for (unsigned b = 0; b < aiM->mNumBones; b++) {
            aiBone *bone = aiM->mBones[b];
            std::string boneName = bone->mName.C_Str();
            if (rt.boneNameToIdx.find(boneName) == rt.boneNameToIdx.end()) {
                int bidx = (int)rt.boneNames.size();
                rt.boneNameToIdx[boneName] = bidx;
                rt.boneNames.push_back(boneName);
                rt.boneOffsets.push_back(AiToRl(bone->mOffsetMatrix));
                // map bone to node
                auto nit = rt.nodeNameToIdx.find(boneName);
                rt.boneToNode.push_back(nit != rt.nodeNameToIdx.end() ? nit->second : -1);
            }
        }
    }

    // ---- 3. 根节点逆矩阵 ----
    Matrix rootM = AiToRl(scene->mRootNode->mTransformation);
    rt.globalInverseRoot = MatrixInvert(rootM);

    // ---- 4. 处理动画通道 ----
    for (unsigned a = 0; a < scene->mNumAnimations; a++) {
        aiAnimation *aiAnim = scene->mAnimations[a];
        AssimpAnimationRuntime::AnimClip clip;
        clip.name = aiAnim->mName.C_Str();

        // 计算最大关键帧数
        int maxFrames = 0;
        for (unsigned c = 0; c < aiAnim->mNumChannels; c++) {
            aiNodeAnim *ch = aiAnim->mChannels[c];
            if (maxFrames < (int)ch->mNumPositionKeys) maxFrames = (int)ch->mNumPositionKeys;
            if (maxFrames < (int)ch->mNumRotationKeys) maxFrames = (int)ch->mNumRotationKeys;
            if (maxFrames < (int)ch->mNumScalingKeys) maxFrames = (int)ch->mNumScalingKeys;
        }
        if (maxFrames < 1) maxFrames = 1;
        clip.keyframeCount = maxFrames;

        // 处理每个通道
        for (unsigned c = 0; c < aiAnim->mNumChannels; c++) {
            aiNodeAnim *ch = aiAnim->mChannels[c];
            std::string nodeName = ch->mNodeName.C_Str();
            auto nit = rt.nodeNameToIdx.find(nodeName);
            if (nit == rt.nodeNameToIdx.end()) continue; // node not in our tree

            AnimChannelData chData;

            // Position keys
            for (unsigned k = 0; k < ch->mNumPositionKeys; k++) {
                chData.posTimes.push_back((float)ch->mPositionKeys[k].mTime);
                chData.posValues.push_back(AiToRl(ch->mPositionKeys[k].mValue));
            }
            // Rotation keys
            for (unsigned k = 0; k < ch->mNumRotationKeys; k++) {
                chData.rotTimes.push_back((float)ch->mRotationKeys[k].mTime);
                chData.rotValues.push_back(AiToRl(ch->mRotationKeys[k].mValue));
            }
            // Scale keys
            for (unsigned k = 0; k < ch->mNumScalingKeys; k++) {
                chData.scaleTimes.push_back((float)ch->mScalingKeys[k].mTime);
                chData.scaleValues.push_back(AiToRl(ch->mScalingKeys[k].mValue));
            }

            clip.channelNode.push_back(nit->second);
            clip.channels.push_back(chData);
        }

        rt.clips.push_back(clip);
    }

    // 分配工作缓冲区
    rt.globalMats.resize(rt.nodes.size());
    rt.skinningMats.resize(rt.boneNames.size());
    rt.valid = true;

    return rt;
}

// ---------------------------------------------------------------------------
// 更新模型动画（Assimp 节点树求值 + CPU skinning）
// ---------------------------------------------------------------------------
void UpdateModelAnimationAssimp(Model *model, AssimpAnimationRuntime *rt, int animClipIdx, float frame) {
    if (!rt->valid || animClipIdx >= (int)rt->clips.size()) return;
    auto &clip = rt->clips[animClipIdx];
    int nNodes = (int)rt->nodes.size();
    int nBones = (int)rt->boneNames.size();
    int maxFrames = clip.keyframeCount;
    if (maxFrames < 1) maxFrames = 1;

    // ---- 1. 计算每个节点的局部矩阵 ----
    // 为每个节点构建局部变换矩阵
    // 先初始化为默认变换
    for (int i = 0; i < nNodes; i++) {
        const AnimNode &node = rt->nodes[i];
        Vector3 pos = node.defaultTransform.translation;
        Quaternion rot = node.defaultTransform.rotation;
        Vector3 sc = node.defaultTransform.scale;

        rt->globalMats[i] = MatrixIdentity();
        // Build SRT: v * S * R * T
        rt->globalMats[i] = MatrixMultiply(MatrixScale(sc.x, sc.y, sc.z), QuaternionToMatrix(rot));
        rt->globalMats[i] = MatrixMultiply(rt->globalMats[i], MatrixTranslate(pos.x, pos.y, pos.z));
    }

    // 用动画数据覆盖有通道的节点
    for (int chIdx = 0; chIdx < (int)clip.channels.size(); chIdx++) {
        int nodeIdx = clip.channelNode[chIdx];
        if (nodeIdx < 0 || nodeIdx >= nNodes) continue;
        const AnimNode &node = rt->nodes[nodeIdx];
        const AnimChannelData &ch = clip.channels[chIdx];
        rt->globalMats[nodeIdx] = InterpolateLocalMatrix(ch, node, frame, maxFrames);
    }

    // ---- 2. 计算全局变换（拓扑有序：父索引 < 子索引） ----
    // node[0] 是根节点（DFS 保证拓扑排序），直接传播
    for (int i = 1; i < nNodes; i++) {
        int parent = rt->nodes[i].parent;
        if (parent >= 0 && parent < nNodes) {
            // raylib row-major: global[child] = local[child] * global[parent]
            rt->globalMats[i] = MatrixMultiply(rt->globalMats[i], rt->globalMats[parent]);
        }
    }

    // ---- 3. 计算蒙皮矩阵 ----
    for (int bi = 0; bi < nBones; bi++) {
        int nodeIdx = rt->boneToNode[bi];
        if (nodeIdx >= 0 && nodeIdx < nNodes) {
            // jointMatrix = nodeGlobal * inverseBindMatrix
            // raylib row-major: v * nodeGlobal * mOffset
            rt->skinningMats[bi] = BuildSkinningMatrix(rt->globalMats[nodeIdx], rt->boneOffsets[bi]);
        } else {
            rt->skinningMats[bi] = MatrixIdentity();
        }
    }

    // ---- 4. CPU skinning（参考 raylib 的 UpdateModelAnimationVertexBuffers） ----
    for (int m = 0; m < model->meshCount; m++) {
        Mesh &mesh = model->meshes[m];
        if (!mesh.boneWeights || !mesh.boneIndices || !mesh.animVertices) continue;

        int vertexValuesCount = mesh.vertexCount * 3;
        bool hasNormals = (mesh.animNormals != NULL);

        for (int v = 0; v < vertexValuesCount; v += 3) {
            Vector3 origV = { mesh.vertices[v], mesh.vertices[v+1], mesh.vertices[v+2] };
            Vector3 origN = {0,0,0};
            if (hasNormals) origN = { mesh.normals[v], mesh.normals[v+1], mesh.normals[v+2] };

            Vector3 newV = {0,0,0};
            Vector3 newN = {0,0,0};
            float totalWeight = 0;

            int boneCounter = v / 3 * 4;
            for (int j = 0; j < 4; j++) {
                float w = mesh.boneWeights[boneCounter + j];
                if (w <= 0) continue;
                int bi = mesh.boneIndices[boneCounter + j];
                if (bi < 0 || bi >= nBones) continue;
                totalWeight += w;

                Vector3 tv = Vector3Transform(origV, rt->skinningMats[bi]);
                newV.x += tv.x * w;
                newV.y += tv.y * w;
                newV.z += tv.z * w;

                if (hasNormals) {
                    Vector3 tn = Vector3Transform(origN, rt->skinningMats[bi]);
                    newN.x += tn.x * w;
                    newN.y += tn.y * w;
                    newN.z += tn.z * w;
                }
            }

            if (totalWeight > 0) {
                mesh.animVertices[v]   = newV.x;
                mesh.animVertices[v+1] = newV.y;
                mesh.animVertices[v+2] = newV.z;
                if (hasNormals) {
                    mesh.animNormals[v]   = newN.x;
                    mesh.animNormals[v+1] = newN.y;
                    mesh.animNormals[v+2] = newN.z;
                }
            } else {
                // No bone influence: keep original (bind pose)
                mesh.animVertices[v]   = origV.x;
                mesh.animVertices[v+1] = origV.y;
                mesh.animVertices[v+2] = origV.z;
            }
        }

        // 上传到 VBO
        if (mesh.animVertices)
            rlUpdateVertexBuffer(mesh.vboId[0], mesh.animVertices, vertexValuesCount * sizeof(float), 0);
        if (hasNormals && mesh.animNormals)
            rlUpdateVertexBuffer(mesh.vboId[2], mesh.animNormals, vertexValuesCount * sizeof(float), 0);
    }

    // 确保 boneMatrices 为 NULL，避免 raylib 的双重蒙皮
    model->boneMatrices = NULL;
}

// ---------------------------------------------------------------------------
// 释放资源
// ---------------------------------------------------------------------------
void UnloadAssimpAnimationRuntime(AssimpAnimationRuntime *rt) {
    rt->nodes.clear();
    rt->nodeNameToIdx.clear();
    rt->boneToNode.clear();
    rt->boneOffsets.clear();
    rt->boneNames.clear();
    rt->boneNameToIdx.clear();
    rt->clips.clear();
    rt->globalMats.clear();
    rt->skinningMats.clear();
    rt->valid = false;
}
