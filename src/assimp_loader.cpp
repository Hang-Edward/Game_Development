#include "assimp_loader.h"
#include "raymath.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <cstring>
#include <map>

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
            int boneIdx = b;
            if (boneNameToIdx) {
                auto it = boneNameToIdx->find(bone->mName.C_Str());
                if (it != boneNameToIdx->end()) boneIdx = it->second;
            }
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
// 递归构建骨骼信息
// ---------------------------------------------------------------------------
static void BuildBoneHierarchy(aiNode *node, const aiScene *scene,
                               std::vector<BoneInfo> &boneInfos,
                               std::vector<Transform> &bindPose,
                               std::map<std::string, int> &nameToIdx,
                               int parentIdx)
{
    // 检查这个 node 是否是动画骨骼
    // Assimp 把骨骼存储为 mesh->mBones[]，但节点本身是场景树的一部分
    // 我们通过遍历 scene->mRootNode 来处理所有节点

    // 先处理当前节点
    std::string name = node->mName.C_Str();
    int myIdx = -1;

    if (nameToIdx.find(name) != nameToIdx.end()) {
        myIdx = nameToIdx[name];
    } else {
        myIdx = (int)boneInfos.size();
        nameToIdx[name] = myIdx;
        BoneInfo bi;
        memset(bi.name, 0, 32);
        strncpy(bi.name, name.c_str(), 31);
        bi.parent = parentIdx;
        boneInfos.push_back(bi);

        // 绑定位姿（从节点变换）
        Transform t;
        aiVector3D pos; aiQuaternion rot; aiVector3D scale;
        node->mTransformation.Decompose(scale, rot, pos);
        t.translation = AiToRl(pos);
        t.rotation = AiToRl(rot);
        t.scale = AiToRl(scale);
        bindPose.push_back(t);
    }

    // 递归处理子节点
    for (unsigned i = 0; i < node->mNumChildren; i++)
        BuildBoneHierarchy(node->mChildren[i], scene, boneInfos, bindPose, nameToIdx, myIdx);
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

    TraceLog(LOG_INFO, "ASSIMP: Loaded %s (%d meshes, %d materials, %d animations, %d textures)",
        fileName, scene->mNumMeshes, scene->mNumMaterials, scene->mNumAnimations, scene->mNumTextures);

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
    // 收集所有涉及骨骼的节点名称
    std::map<std::string, int> boneNameToIdx;
    std::vector<BoneInfo> boneInfos;
    std::vector<Transform> bindPoses;

    // 从所有网格收集骨骼名称
    for (unsigned i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *aiM = scene->mMeshes[i];
        if (!aiM->HasBones()) continue;
        for (unsigned b = 0; b < aiM->mNumBones; b++) {
            std::string name = aiM->mBones[b]->mName.C_Str();
            if (boneNameToIdx.find(name) == boneNameToIdx.end()) {
                int idx = (int)boneInfos.size();
                boneNameToIdx[name] = idx;
                BoneInfo bi;
                memset(bi.name, 0, 32);
                strncpy(bi.name, name.c_str(), 31);
                bi.parent = -1; // 稍后修正
                boneInfos.push_back(bi);

    // -- 网格 --
    model.meshCount = (int)scene->mNumMeshes;
    model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));

    for (unsigned i = 0; i < scene->mNumMeshes; i++) {
        model.meshes[i] = aiMeshToRaylib(scene->mMeshes[i], scene, &boneNameToIdx);
        // 每个网格引用自己的材质
        model.meshMaterial[i] = (int)scene->mMeshes[i]->mMaterialIndex;
    }
                // 绑定位姿：从骨骼的逆绑定矩阵反算
                aiMatrix4x4 invBind = aiM->mBones[b]->mOffsetMatrix;
                aiMatrix4x4 bind = invBind;
                bind.Inverse();
                aiVector3D pos; aiQuaternion rot; aiVector3D scale;
                bind.Decompose(scale, rot, pos);
                Transform t;
                t.translation = AiToRl(pos);
                t.rotation = AiToRl(rot);
                t.scale = AiToRl(scale);
                bindPoses.push_back(t);
            }
        }
    }

    // -- Fix bone parents from scene tree --
    struct StackFrame { aiNode *node; int parentBoneIdx; };
    std::vector<StackFrame> stack;
    stack.push_back({ scene->mRootNode, -1 });
    while (!stack.empty()) {
        StackFrame f = stack.back();
        stack.pop_back();
        std::string name = f.node->mName.C_Str();
        auto it = boneNameToIdx.find(name);
        int myBoneIdx = (it != boneNameToIdx.end()) ? it->second : -1;
        if (myBoneIdx >= 0 && f.parentBoneIdx >= 0 && boneInfos[myBoneIdx].parent < 0)
            boneInfos[myBoneIdx].parent = f.parentBoneIdx;
        for (unsigned ci = 0; ci < f.node->mNumChildren; ci++)
            stack.push_back({ f.node->mChildren[ci], (myBoneIdx >= 0) ? myBoneIdx : f.parentBoneIdx });
    }
    // 递归遍历场景树，为 boneInfos 中的每个节点找父亲


    // -- 根节点变换（标准化去缩放） --
    if (scene->mRootNode) {
        Matrix m = AiToRl(scene->mRootNode->mTransformation);
        Vector3 c0 = Vector3Normalize({m.m0, m.m1, m.m2});
        Vector3 c1 = Vector3Normalize({m.m4, m.m5, m.m6});
        Vector3 c2 = Vector3Normalize({m.m8, m.m9, m.m10});
        float sc = (Vector3Length({m.m0,m.m1,m.m2}) +
                    Vector3Length({m.m4,m.m5,m.m6}) +
                    Vector3Length({m.m8,m.m9,m.m10})) / 3.0f;
        if (fabsf(sc - 1.0f) > 0.001f && model.skeleton.boneCount > 0) {
            for (int i = 0; i < model.skeleton.boneCount; i++) {
                model.skeleton.bindPose[i].scale.x *= sc;
                model.skeleton.bindPose[i].scale.y *= sc;
                model.skeleton.bindPose[i].scale.z *= sc;
            }
        }
        model.transform = MatrixIdentity();
        model.transform.m0 = c0.x; model.transform.m1 = c0.y; model.transform.m2 = c0.z;
        model.transform.m4 = c1.x; model.transform.m5 = c1.y; model.transform.m6 = c1.z;
        model.transform.m8 = c2.x; model.transform.m9 = c2.y; model.transform.m10 = c2.z;
        model.transform.m12 = m.m12; model.transform.m13 = m.m13; model.transform.m14 = m.m14;
        model.transform.m15 = m.m15;
    }

    // 写入 Model 骨架
    model.skeleton.boneCount = (int)boneInfos.size();
    if (model.skeleton.boneCount > 0) {
        model.skeleton.bones = (BoneInfo *)RL_CALLOC(model.skeleton.boneCount, sizeof(BoneInfo));
        memcpy(model.skeleton.bones, boneInfos.data(), model.skeleton.boneCount * sizeof(BoneInfo));
        // bindPose
        model.skeleton.bindPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));
        memcpy(model.skeleton.bindPose, bindPoses.data(), model.skeleton.boneCount * sizeof(Transform));

        // currentPose
        model.currentPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));
        memcpy(model.currentPose, bindPoses.data(), model.skeleton.boneCount * sizeof(Transform));

        // boneMatrices
        model.boneMatrices = (Matrix *)RL_CALLOC(model.skeleton.boneCount, sizeof(Matrix));
        for (int i = 0; i < model.skeleton.boneCount; i++)
            model.boneMatrices[i] = MatrixIdentity();
    }

    TraceLog(LOG_INFO, "ASSIMP: Model loaded: %d bones", model.skeleton.boneCount);
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

    // 先建立 bone name → index 映射（和 LoadModelAssimp 一致的方式）
    std::map<std::string, int> boneMap;
    for (unsigned m = 0; m < scene->mNumMeshes; m++) {
        aiMesh *aiM = scene->mMeshes[m];
        if (!aiM->HasBones()) continue;
        for (unsigned b = 0; b < aiM->mNumBones; b++) {
            std::string name = aiM->mBones[b]->mName.C_Str();
            if (boneMap.find(name) == boneMap.end())
                boneMap[name] = (int)boneMap.size();
        }
    }

    // 为每段动画创建一个 ModelAnimation
    *animCount = (int)scene->mNumAnimations;
    ModelAnimation *anims = (ModelAnimation *)RL_CALLOC(*animCount, sizeof(ModelAnimation));

    for (unsigned a = 0; a < scene->mNumAnimations; a++) {
        aiAnimation *aiAnim = scene->mAnimations[a];
        ModelAnimation *rlAnim = &anims[a];

        memset(rlAnim->name, 0, 32);
        strncpy(rlAnim->name, aiAnim->mName.C_Str(), 31);
        rlAnim->boneCount = (int)boneMap.size();

        // 计算帧数（取所有 channel 的最大关键帧数）
        int maxFrames = 0;
        for (unsigned c = 0; c < aiAnim->mNumChannels; c++) {
            aiNodeAnim *ch = aiAnim->mChannels[c];
            maxFrames = std::max(maxFrames, (int)ch->mNumPositionKeys);
            maxFrames = std::max(maxFrames, (int)ch->mNumRotationKeys);
            maxFrames = std::max(maxFrames, (int)ch->mNumScalingKeys);
        }
        if (maxFrames < 1) maxFrames = 1;

        rlAnim->keyframeCount = maxFrames;
        rlAnim->keyframePoses = (ModelAnimPose *)RL_CALLOC(maxFrames, sizeof(ModelAnimPose)); // each frame: Transform[]

        // 为每帧分配 Transform 数组
        for (int f = 0; f < maxFrames; f++) {
            rlAnim->keyframePoses[f] = (Transform *)RL_CALLOC(rlAnim->boneCount, sizeof(Transform));
            // 默认值
            for (int i = 0; i < rlAnim->boneCount; i++) {
                rlAnim->keyframePoses[f][i].translation = {0,0,0};
                rlAnim->keyframePoses[f][i].rotation = {0,0,0,1};
                rlAnim->keyframePoses[f][i].scale = {1,1,1};
            }
        }

        // 填充每个 channel 的关键帧数据
        for (unsigned c = 0; c < aiAnim->mNumChannels; c++) {
            aiNodeAnim *ch = aiAnim->mChannels[c];
            std::string nodeName = ch->mNodeName.C_Str();
            auto it = boneMap.find(nodeName);
            if (it == boneMap.end()) continue;
            int boneIdx = it->second;

            for (int f = 0; f < maxFrames; f++) {
                Transform *t = &rlAnim->keyframePoses[f][boneIdx];

                // Translation
                if (ch->mNumPositionKeys > 0) {
                    unsigned keyIdx = (ch->mNumPositionKeys == 1) ? 0
                        : (unsigned)((float)f / (maxFrames-1) * (ch->mNumPositionKeys-1) + 0.5f);
                    t->translation = AiToRl(ch->mPositionKeys[keyIdx].mValue);
                }
                // Rotation
                if (ch->mNumRotationKeys > 0) {
                    unsigned keyIdx = (ch->mNumRotationKeys == 1) ? 0
                        : (unsigned)((float)f / (maxFrames-1) * (ch->mNumRotationKeys-1) + 0.5f);
                    t->rotation = AiToRl(ch->mRotationKeys[keyIdx].mValue);
                }
                // Scale（Assimp 正确处理 scale，不会压扁模型）
                if (ch->mNumScalingKeys > 0) {
                    unsigned keyIdx = (ch->mNumScalingKeys == 1) ? 0
                        : (unsigned)((float)f / (maxFrames-1) * (ch->mNumScalingKeys-1) + 0.5f);
                    t->scale = AiToRl(ch->mScalingKeys[keyIdx].mValue);
                }
            }
        }
    }

    TraceLog(LOG_INFO, "ASSIMP: Loaded %d animations (%d bones)", *animCount, (int)boneMap.size());
    return anims;
}
