// GLB diagnostic tool - checks character model files for issues
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstdio>
#include <cmath>
#include <string>
#include <set>
#include <map>
#include <vector>

static void AiToRlMatrix(const aiMatrix4x4 &m, float out[16]) {
    out[0]=m.a1; out[1]=m.b1; out[2]=m.c1; out[3]=m.d1;
    out[4]=m.a2; out[5]=m.b2; out[6]=m.c2; out[7]=m.d2;
    out[8]=m.a3; out[9]=m.b3; out[10]=m.c3; out[11]=m.d3;
    out[12]=m.a4; out[13]=m.b4; out[14]=m.c4; out[15]=m.d4;
}

static bool HasNaN(const float *m, int n) {
    for (int i = 0; i < n; i++)
        if (std::isnan(m[i]) || std::isinf(m[i])) return true;
    return false;
}

int main(int argc, char *argv[]) {
    const char *files[] = {
        "../assets/models/character/stand.glb",
        "../assets/models/character/walk.glb",
        "../assets/models/character/run.glb"
    };

    for (int fi = 0; fi < 3; fi++) {
        const char *fname = files[fi];
        printf("\n========================================\n");
        printf("FILE: %s\n", fname);
        printf("========================================\n");

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(fname,
            aiProcess_Triangulate | aiProcess_GenNormals |
            aiProcess_LimitBoneWeights | aiProcess_FlipUVs);

        if (!scene || !scene->mRootNode) {
            printf("ERROR: Failed to load: %s\n", importer.GetErrorString());
            continue;
        }

        printf("Meshes: %d\n", scene->mNumMeshes);
        printf("Materials: %d\n", scene->mNumMaterials);
        printf("Animations: %d\n", scene->mNumAnimations);
        printf("Textures: %d\n", scene->mNumTextures);

        // ---- Mesh info ----
        printf("\n--- MESHES ---\n");
        for (unsigned i = 0; i < scene->mNumMeshes; i++) {
            aiMesh *m = scene->mMeshes[i];
            printf("mesh[%d]: %d verts, %d faces, %d bones",
                i, m->mNumVertices, m->mNumFaces, m->mNumBones);
            if (m->HasBones()) {
                printf(" [");
                for (unsigned b = 0; b < m->mNumBones; b++)
                    printf("%s%s", b ? "," : "", m->mBones[b]->mName.C_Str());
                printf("]");
            }
            printf("\n");
        }

        // ---- Bone hierarchy from scene graph ----
        printf("\n--- SCENE GRAPH NODES ---\n");
        struct SFrame { aiNode *node; int depth; };
        std::vector<SFrame> sstack;
        sstack.push_back({scene->mRootNode, 0});
        while (!sstack.empty()) {
            SFrame f = sstack.back(); sstack.pop_back();
            for (int i = 0; i < f.depth; i++) printf("  ");
            printf("%s", f.node->mName.C_Str());
            // show transform
            aiVector3D pos, sca; aiQuaternion rot;
            f.node->mTransformation.Decompose(sca, rot, pos);
            printf(" t=(%.4f,%.4f,%.4f) r=(%.4f,%.4f,%.4f,%.4f) s=(%.4f,%.4f,%.4f)",
                pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, rot.w, sca.x, sca.y, sca.z);
            // check for NaN
            float m[16]; AiToRlMatrix(f.node->mTransformation, m);
            if (HasNaN(m, 16)) printf(" <<< NAN/INF IN NODE TRANSFORM");
            printf("\n");
            for (unsigned ci = 0; ci < f.node->mNumChildren; ci++)
                sstack.push_back({f.node->mChildren[ci], f.depth + 1});
        }

        // ---- Collect all bone names from all meshes ----
        printf("\n--- BONES (from mesh skin data) ---\n");
        std::map<std::string, std::pair<int, aiMatrix4x4>> allBones; // name -> (mesh, offset)
        std::map<std::string, int> boneCount;
        for (unsigned i = 0; i < scene->mNumMeshes; i++) {
            aiMesh *m = scene->mMeshes[i];
            if (!m->HasBones()) continue;
            for (unsigned b = 0; b < m->mNumBones; b++) {
                std::string n = m->mBones[b]->mName.C_Str();
                boneCount[n]++;
                if (allBones.find(n) == allBones.end())
                    allBones[n] = {i, m->mBones[b]->mOffsetMatrix};
            }
        }
        printf("Total unique bone names: %d\n", (int)allBones.size());

        // Check each bone's offset matrix for NaN and extreme values
        printf("\n--- BIND POSE CHECK (from mOffsetMatrix.Inverse) ---\n");
        int nanCount = 0, extremeCount = 0;
        for (auto &kv : allBones) {
            aiMatrix4x4 offset = kv.second.second;
            aiMatrix4x4 bind = offset;
            bind.Inverse();
            aiVector3D pos, sca; aiQuaternion rot;
            bind.Decompose(sca, rot, pos);
            float m[16]; AiToRlMatrix(bind, m);
            bool hasNaN = HasNaN(m, 16);
            bool extreme = (fabsf(sca.x) > 100 || fabsf(sca.y) > 100 || fabsf(sca.z) > 100 ||
                           fabsf(pos.x) > 1000 || fabsf(pos.y) > 1000 || fabsf(pos.z) > 1000);
            if (hasNaN || extreme) {
                printf("  %s: mesh[%d]", kv.first.c_str(), kv.second.first);
                if (hasNaN) { printf(" NaN/INF"); nanCount++; }
                if (extreme) { printf(" EXTREME(s=%.2f,%.2f,%.2f t=%.2f,%.2f,%.2f)",
                    sca.x, sca.y, sca.z, pos.x, pos.y, pos.z); extremeCount++; }
                printf("\n");
            }
        }
        if (nanCount == 0 && extremeCount == 0)
            printf("  All bones OK (no NaN, no extreme values)\n");
        else
            printf("  WARNING: %d NaN/INF, %d extreme values\n", nanCount, extremeCount);

        // Check for bones referenced by multiple meshes
        printf("\n--- BONE SHARING (bones in >1 mesh) ---\n");
        int shared = 0;
        for (auto &kv : boneCount) {
            if (kv.second > 1) {
                printf("  %s: appears in %d meshes\n", kv.first.c_str(), kv.second);
                shared++;
            }
        }
        if (shared == 0) printf("  No shared bones (each bone in exactly 1 mesh)\n");

        // ---- Animation channels ----
        if (scene->mNumAnimations > 0) {
            printf("\n--- ANIMATIONS ---\n");
            for (unsigned a = 0; a < scene->mNumAnimations; a++) {
                aiAnimation *anim = scene->mAnimations[a];
                printf("anim[%d]: '%s' %d channels, duration=%.3f, ticks/sec=%.3f\n",
                    a, anim->mName.C_Str(), anim->mNumChannels,
                    anim->mDuration, anim->mTicksPerSecond);

                // Check each channel for NaN values
                int nanCh = 0;
                for (unsigned c = 0; c < anim->mNumChannels; c++) {
                    aiNodeAnim *ch = anim->mChannels[c];
                    // check first key of each type
                    if (ch->mNumPositionKeys > 0) {
                        auto &v = ch->mPositionKeys[0].mValue;
                        if (std::isnan(v.x) || std::isinf(v.x) ||
                            std::isnan(v.y) || std::isinf(v.y) ||
                            std::isnan(v.z) || std::isinf(v.z)) {
                            if (nanCh == 0) printf("  NaN in position keys:\n");
                            printf("    channel '%s' pos key 0\n", ch->mNodeName.C_Str());
                            nanCh++;
                        }
                    }
                    if (ch->mNumRotationKeys > 0) {
                        auto &q = ch->mRotationKeys[0].mValue;
                        if (std::isnan(q.x) || std::isinf(q.x)) {
                            if (nanCh == 0) printf("  NaN in rotation keys:\n");
                            printf("    channel '%s' rot key 0\n", ch->mNodeName.C_Str());
                            nanCh++;
                        }
                    }
                    if (ch->mNumScalingKeys > 0) {
                        auto &v = ch->mScalingKeys[0].mValue;
                        if (std::isnan(v.x) || std::isinf(v.x)) {
                            if (nanCh == 0) printf("  NaN in scaling keys:\n");
                            printf("    channel '%s' scale key 0\n", ch->mNodeName.C_Str());
                            nanCh++;
                        }
                    }
                    // check keyframe count consistency
                    int nk = (int)ch->mNumPositionKeys;
                    if (nk > 0 && ch->mNumRotationKeys > 0 && ch->mNumScalingKeys > 0) {
                        if ((int)ch->mNumPositionKeys != (int)ch->mNumRotationKeys ||
                            (int)ch->mNumPositionKeys != (int)ch->mNumScalingKeys) {
                            printf("  WARNING: channel '%s' has %d pos, %d rot, %d scale keys\n",
                                ch->mNodeName.C_Str(),
                                (int)ch->mNumPositionKeys, (int)ch->mNumRotationKeys, (int)ch->mNumScalingKeys);
                        }
                    }
                }
                if (nanCh == 0) printf("  No NaN/INF in animation data\n");

                // Print keyframe values for key bones
                const char *keyBones[] = {"bip001_pelvis_07", "bip001_spine2_010",
                    "bip001_head_012", "bip001_r_hand_057", "b_wp_1_058", "scythe_059"};
                for (unsigned c = 0; c < anim->mNumChannels; c++) {
                    aiNodeAnim *ch = anim->mChannels[c];
                    for (unsigned kb = 0; kb < sizeof(keyBones)/sizeof(keyBones[0]); kb++) {
                        if (strcmp(ch->mNodeName.C_Str(), keyBones[kb]) == 0) {
                            printf("  Channel '%s': ", keyBones[kb]);
                            if (ch->mNumPositionKeys > 0) {
                                if (ch->mNumPositionKeys == 1) {
                                    printf("pos=(%.4f,%.4f,%.4f)[1 key] ",
                                        ch->mPositionKeys[0].mValue.x,
                                        ch->mPositionKeys[0].mValue.y,
                                        ch->mPositionKeys[0].mValue.z);
                                } else {
                                    printf("pos=(%.4f,%.4f,%.4f)->(%.4f,%.4f,%.4f)[%d keys] ",
                                        ch->mPositionKeys[0].mValue.x,
                                        ch->mPositionKeys[0].mValue.y,
                                        ch->mPositionKeys[0].mValue.z,
                                        ch->mPositionKeys[ch->mNumPositionKeys-1].mValue.x,
                                        ch->mPositionKeys[ch->mNumPositionKeys-1].mValue.y,
                                        ch->mPositionKeys[ch->mNumPositionKeys-1].mValue.z,
                                        ch->mNumPositionKeys);
                                }
                            } else printf("pos=NONE ");
                            if (ch->mNumRotationKeys > 0)
                                printf("rot[%d]", ch->mNumRotationKeys);
                            if (ch->mNumScalingKeys > 0) {
                                if (ch->mNumScalingKeys == 1) {
                                    printf(" scale=(%.4f,%.4f,%.4f)",
                                        ch->mScalingKeys[0].mValue.x,
                                        ch->mScalingKeys[0].mValue.y,
                                        ch->mScalingKeys[0].mValue.z);
                                }
                            }
                            printf("\n");
                            break;
                        }
                    }
                }

                // Check if channel names match scene graph nodes
                printf("  Scene graph node match check:\n");
                int match = 0, noMatch = 0;
                for (unsigned c = 0; c < anim->mNumChannels; c++) {
                    aiNodeAnim *ch = anim->mChannels[c];
                    std::string chName = ch->mNodeName.C_Str();
                    // search scene graph for this name
                    bool found = false;
                    std::vector<aiNode*> stack;
                    stack.push_back(scene->mRootNode);
                    while (!stack.empty()) {
                        aiNode *n = stack.back(); stack.pop_back();
                        if (chName == n->mName.C_Str()) { found = true; break; }
                        for (unsigned ci = 0; ci < n->mNumChildren; ci++)
                            stack.push_back(n->mChildren[ci]);
                    }
                    if (found) match++; else {
                        if (noMatch < 5)
                            printf("    MISSING: '%s' not in scene graph!\n", chName.c_str());
                        noMatch++;
                    }
                }
                printf("  Channel->node match: %d/%d", match, match + noMatch);
                if (noMatch > 0) printf(" (%d unmatched)", noMatch);
                printf("\n");
            }
        }

        printf("\n");
    }

    return 0;
}
