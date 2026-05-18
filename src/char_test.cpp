// Standalone character model viewer - no map, no terrain
#include "raylib.h"
#include "raymath.h"
#include "assimp_loader.h"
#include <cmath>

int main() {
    const int W = 1280, H = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(W, H, "Character Test");

    Camera3D cam = {0};
    cam.position = {3, 2, 3};
    cam.target = {0, 0.8f, 0};
    cam.up = {0, 1, 0};
    cam.fovy = 60;
    cam.projection = CAMERA_PERSPECTIVE;

    // Load character models
    Model standModel = LoadModelAssimp("../assets/models/character/stand.glb");
    Model walkModel  = LoadModelAssimp("../assets/models/character/walk.glb");
    Model runModel   = LoadModelAssimp("../assets/models/character/run.glb");

    int standCount = 0, walkCount = 0, runCount = 0;
    ModelAnimation *standAnims = LoadModelAnimationsAssimp("../assets/models/character/stand.glb", &standCount);
    ModelAnimation *walkAnims  = LoadModelAnimationsAssimp("../assets/models/character/walk.glb", &walkCount);
    ModelAnimation *runAnims   = LoadModelAnimationsAssimp("../assets/models/character/run.glb", &runCount);

    // Fix animation keyframes: set translations to bindPose (files have no position animation)
    if (standCount > 0 && standModel.skeleton.boneCount > 0)
        FixAnimationPose(standModel, standAnims[0]);
    if (walkCount > 0 && walkModel.skeleton.boneCount > 0)
        FixAnimationPose(walkModel, walkAnims[0]);
    if (runCount > 0 && runModel.skeleton.boneCount > 0)
        FixAnimationPose(runModel, runAnims[0]);

    Matrix *standMat = standModel.boneMatrices;
    Matrix *walkMat  = walkModel.boneMatrices;
    Matrix *runMat   = runModel.boneMatrices;

    bool hasChar = (standModel.skeleton.boneCount > 0);
    Model *activeModel = hasChar ? &standModel : NULL;

    // (animation fix is now done by FixAnimationPose above)

    // Find the first frame where ANY bone has non-zero translation
    if (standCount > 0) {
        int foundFrame = -1;
        for (int f = 0; f < standAnims[0].keyframeCount; f++) {
            bool hasTranslation = false;
            for (int bi = 0; bi < standAnims[0].boneCount; bi++) {
                Transform *kp = &standAnims[0].keyframePoses[f][bi];
                if (fabsf(kp->translation.x) > 0.001f ||
                    fabsf(kp->translation.y) > 0.001f ||
                    fabsf(kp->translation.z) > 0.001f) {
                    hasTranslation = true;
                    break;
                }
            }
            if (hasTranslation) { foundFrame = f; break; }
        }
        TraceLog(LOG_WARNING, "First anim frame with non-zero translation: %d", foundFrame);
    }

    TraceLog(LOG_WARNING, "stand bones=%d anim bones=%d frames=%d",
        standModel.skeleton.boneCount, standCount > 0 ? standAnims[0].boneCount : 0,
        standCount > 0 ? standAnims[0].keyframeCount : 0);

    // Debug: print first 5 bones' first-frame animation data
    if (standCount > 0) {
        for (int i = 0; i < 5 && i < standAnims[0].boneCount; i++) {
            Transform *kp = &standAnims[0].keyframePoses[0][i];
            // convert from model space back to printable: just show raw values
            TraceLog(LOG_WARNING, "  anim[0] bone[%d] frame0: t=(%.4f,%.4f,%.4f) r=(%.4f,%.4f,%.4f,%.4f) s=(%.4f,%.4f,%.4f)",
                i, kp->translation.x, kp->translation.y, kp->translation.z,
                kp->rotation.x, kp->rotation.y, kp->rotation.z, kp->rotation.w,
                kp->scale.x, kp->scale.y, kp->scale.z);
        }
        // Also check the bind pose for same bones for comparison
        for (int i = 0; i < 5 && i < standModel.skeleton.boneCount; i++) {
            Transform *bp = &standModel.skeleton.bindPose[i];
            TraceLog(LOG_WARNING, "  bind[%d]: t=(%.4f,%.4f,%.4f) s=(%.4f,%.4f,%.4f)",
                i, bp->translation.x, bp->translation.y, bp->translation.z,
                bp->scale.x, bp->scale.y, bp->scale.z);
        }
    }

    // frame range
    int startFrame = 0, endFrame = 0;
    if (standCount > 0) endFrame = standAnims[0].keyframeCount - 1;
    int animFrame = 0;

    DisableCursor();

    float camAngleX = 0, camAngleY = 20, camDist = 4;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_ESCAPE)) EnableCursor();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && IsCursorHidden() == false) DisableCursor();

        if (IsCursorHidden()) {
            Vector2 md = GetMouseDelta();
            camAngleX -= md.x * 0.2f;
            camAngleY += md.y * 0.2f;
            camAngleY = Clamp(camAngleY, -30, 85);
            camDist = Clamp(camDist - GetMouseWheelMove() * 0.5f, 1.5f, 10);
        }

        // switch animation with keys
        ModelAnimation *cur = NULL;
        if (IsKeyDown(KEY_TWO) && walkCount > 0) {
            activeModel = &walkModel; cur = &walkAnims[0];
            startFrame = 0; endFrame = walkAnims[0].keyframeCount - 1;
        } else if (IsKeyDown(KEY_THREE) && runCount > 0) {
            activeModel = &runModel; cur = &runAnims[0];
            startFrame = 0; endFrame = runAnims[0].keyframeCount - 1;
        } else if (standCount > 0) {
            activeModel = &standModel; cur = &standAnims[0];
            startFrame = 0; endFrame = standAnims[0].keyframeCount - 1;
        }

        // animate
        if (cur && endFrame > startFrame) {
            animFrame = startFrame + ((animFrame - startFrame + 1) % (endFrame - startFrame + 1));
            if (animFrame < startFrame) animFrame = startFrame;

            Matrix *saved = (activeModel == &standModel) ? standMat :
                           (activeModel == &walkModel) ? walkMat : runMat;
            activeModel->boneMatrices = saved;
            UpdateModelAnimation(*activeModel, *cur, (float)animFrame);
            activeModel->boneMatrices = NULL;
        }

        // orbit camera
        float yaw = camAngleX * DEG2RAD, pitch = camAngleY * DEG2RAD;
        cam.position = {
            camDist * cosf(pitch) * sinf(yaw) * -1,
            camDist * sinf(pitch) + 0.6f,
            camDist * cosf(pitch) * cosf(yaw) * -1
        };
        cam.target = {0, 0.8f, 0};

        BeginDrawing();
        ClearBackground({60, 60, 80, 255});

        // grid floor
        BeginMode3D(cam);
        DrawGrid(20, 0.5f);

        if (hasChar && activeModel) {
            DrawModelEx(*activeModel, {0, 0, 0}, {0, 1, 0}, 0, {0.075f, 0.075f, 0.075f}, WHITE);
        }

        EndMode3D();

        DrawText("Press 1=stand 2=walk 3=run", 20, 20, 18, WHITE);
        DrawText(TextFormat("Bones: %d  Frame: %d/%d  MeshFilter: see collectMeshes[]",
            activeModel ? activeModel->skeleton.boneCount : 0,
            animFrame, endFrame), 20, 45, 15, Fade(WHITE, 0.7f));

        EndDrawing();
    }

    if (hasChar) {
        UnloadModel(standModel);
        UnloadModel(walkModel);
        UnloadModel(runModel);
        if (standAnims) UnloadModelAnimations(standAnims, standCount);
        if (walkAnims)  UnloadModelAnimations(walkAnims, walkCount);
        if (runAnims)   UnloadModelAnimations(runAnims, runCount);
    }
    CloseWindow();
    return 0;
}
