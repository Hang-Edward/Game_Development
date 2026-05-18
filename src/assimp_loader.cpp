#include "assimp_loader.h"
#include "raymath.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

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

    // 骨骼数据（骨骼索引 + 权重）
    if (aiM->HasBones()) {
        // 每个顶点最多 4 个骨骼影响
        int maxBones = 4;
        m.boneIndices = (unsigned char *)RL_CALLOC(m.vertexCount * 4, sizeof(unsigned char));
        m.boneWeights = (float *)RL_CALLOC(m.vertexCount * 4, sizeof(float));

        for (unsigned b = 0; b < aiM->mNumBones; b++) {
            aiBone *bone = aiM->mBones[b];
            int boneIdx = -1;
            if (boneNameToIdx) {
                auto it = boneNameToIdx->find(bone->mName.C_Str());
                if (it != boneNameToIdx->end()) boneIdx = it->second;
            }
            if (boneIdx < 0) continue; // 跳过不在主骨架中的骨骼（如影子捕捉器）
            for (unsigned w = 0; w < bone->mNumWeights; w++) {
                aiVertexWeight *vw = &bone->mWeights[w];
                unsigned vid = vw->mVertexId;
                // 找到该顶点的第一个空槽
                for (int slot = 0; slot < maxBones; slot++) {
                    if (m.boneWeights[vid * 4 + slot] == 0.0f) {
                        m.boneIndices[vid * 4 + slot] = (unsigned char)boneIdx;
                        m.boneWeights[vid * 4 + slot] = vw->mWeight;
                        break;
                    }
                }
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
        for (unsigned b = 0; b < aiM->mNumBones; b++)
            allBoneNames.insert(aiM->mBones[b]->mName.C_Str());
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
