// Standalone character model viewer - uses Assimp node-tree animation evaluation
#include "raylib.h"
#include "raymath.h"
#include "assimp_loader.h"
#include <cmath>

int main() {
    const int W = 1280, H = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(W, H, "Character Test (Assimp Anim)");

    Camera3D cam = {0};
    cam.position = {3, 2, 3};
    cam.target = {0, 0.8f, 0};
    cam.up = {0, 1, 0};
    cam.fovy = 60;
    cam.projection = CAMERA_PERSPECTIVE;

    // Load model meshes + skeleton (using raylib model for mesh data/rendering)
    Model standModel = LoadModelAssimp("../assets/models/character/stand.glb");
    Model walkModel  = LoadModelAssimp("../assets/models/character/walk.glb");
    Model runModel   = LoadModelAssimp("../assets/models/character/run.glb");

    // Load Assimp animation runtimes (node tree + channels + bone mapping)
    AssimpAnimationRuntime standRT = LoadAssimpAnimationRuntime("../assets/models/character/stand.glb");
    AssimpAnimationRuntime walkRT  = LoadAssimpAnimationRuntime("../assets/models/character/walk.glb");
    AssimpAnimationRuntime runRT   = LoadAssimpAnimationRuntime("../assets/models/character/run.glb");

    bool hasChar = (standModel.skeleton.boneCount > 0);
    Model *activeModel = &standModel;
    AssimpAnimationRuntime *activeRT = standRT.valid ? &standRT : NULL;

    int animFrame = 0;

    // save boneMatrices pointer to prevent double-skinning
    Matrix *standMat = standModel.boneMatrices;
    Matrix *walkMat  = walkModel.boneMatrices;
    Matrix *runMat   = runModel.boneMatrices;

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
        if (IsKeyDown(KEY_TWO) && walkRT.valid) {
            activeModel = &walkModel; activeRT = &walkRT;
        } else if (IsKeyDown(KEY_THREE) && runRT.valid) {
            activeModel = &runModel; activeRT = &runRT;
        } else if (standRT.valid) {
            activeModel = &standModel; activeRT = &standRT;
        }

        // animate using Assimp node-tree evaluation
        if (activeRT) {
            int maxF = activeRT->clips[0].keyframeCount;
            animFrame = (animFrame + 1) % maxF;
            UpdateModelAnimationAssimp(activeModel, activeRT, 0, (float)animFrame);
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

        BeginMode3D(cam);
        DrawGrid(20, 0.5f);

        if (hasChar && activeModel) {
            DrawModelEx(*activeModel, {0, 0, 0}, {0, 1, 0}, 0, {0.075f, 0.075f, 0.075f}, WHITE);
        }

        EndMode3D();

        DrawText("Press 1=stand 2=walk 3=run", 20, 20, 18, WHITE);
        DrawText(TextFormat("Frame: %d/%d  Bones: %d",
            animFrame, activeRT ? activeRT->clips[0].keyframeCount : 0,
            standModel.skeleton.boneCount), 20, 45, 15, Fade(WHITE, 0.7f));

        EndDrawing();
    }

    if (standRT.valid) UnloadAssimpAnimationRuntime(&standRT);
    if (walkRT.valid)  UnloadAssimpAnimationRuntime(&walkRT);
    if (runRT.valid)   UnloadAssimpAnimationRuntime(&runRT);

    if (hasChar) {
        UnloadModel(standModel);
        UnloadModel(walkModel);
        UnloadModel(runModel);
    }
    CloseWindow();
    return 0;
}
