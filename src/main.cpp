// 魔法碎片：暗蚀纪元
#include "raylib.h"
#include "raymath.h"
#include "assimp_loader.h"
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>

// ---------------------------------------------------------------------------
// 地形碰撞
// ---------------------------------------------------------------------------
struct TerrainCollision {
    std::vector<std::vector<float>> heightGrid;
    int resX = 0, resZ = 0;
    float worldMinX = 0, worldMaxX = 0, worldMinZ = 0, worldMaxZ = 0;

    void BuildFromModel(Model model, int res) {
        resX = resZ = res;
        Matrix mat = model.transform;
        BoundingBox box = GetModelBoundingBox(model);
        worldMinX = box.min.x; worldMaxX = box.max.x;
        worldMinZ = box.min.z; worldMaxZ = box.max.z;
        float spanX = worldMaxX - worldMinX, spanZ = worldMaxZ - worldMinZ;

        struct B2 { float minX, maxX, minZ, maxZ; };
        std::vector<B2> mb(model.meshCount, {1e9f,-1e9f,1e9f,-1e9f});
        for (int m = 0; m < model.meshCount; m++) {
            Mesh *mesh = &model.meshes[m];
            if (!mesh->vertices) continue;
            for (int v = 0; v < mesh->vertexCount; v++) {
                Vector3 p = Vector3Transform({mesh->vertices[v*3],mesh->vertices[v*3+1],mesh->vertices[v*3+2]}, mat);
                if (p.x < mb[m].minX) mb[m].minX = p.x; if (p.x > mb[m].maxX) mb[m].maxX = p.x;
                if (p.z < mb[m].minZ) mb[m].minZ = p.z; if (p.z > mb[m].maxZ) mb[m].maxZ = p.z;
            }
        }
        heightGrid.assign(resZ, std::vector<float>(resX, -1e9f));
        for (int m = 0; m < model.meshCount; m++) {
            if (mb[m].minX > mb[m].maxX) continue;
            if (mb[m].maxX - mb[m].minX < spanX * 0.4f || mb[m].maxZ - mb[m].minZ < spanZ * 0.4f) continue;
            Mesh *mesh = &model.meshes[m];
            for (int v = 0; v < mesh->vertexCount; v++) {
                Vector3 p = Vector3Transform({mesh->vertices[v*3],mesh->vertices[v*3+1],mesh->vertices[v*3+2]}, mat);
                int gx = Clamp((int)((p.x-worldMinX)/spanX*resX), 0, resX-1);
                int gz = Clamp((int)((p.z-worldMinZ)/spanZ*resZ), 0, resZ-1);
                if (p.y > heightGrid[gz][gx]) heightGrid[gz][gx] = p.y;
            }
        }
        for (int z = 0; z < resZ; z++) for (int x = 0; x < resX; x++)
            if (heightGrid[z][x] < -1e8f) {
                float h = 0; int c = 0;
                for (int dz = -2; dz <= 2; dz++) for (int dx = -2; dx <= 2; dx++) {
                    int nx = x+dx, nz = z+dz;
                    if (nx>=0 && nx<resX && nz>=0 && nz<resZ && heightGrid[nz][nx] > -1e8f) { h += heightGrid[nz][nx]; c++; }
                }
                heightGrid[z][x] = (c > 0) ? h/c : 0;
            }
    }
    float GetHeight(float wx, float wz) const {
        float u = Clamp((wx-worldMinX)/(worldMaxX-worldMinX), 0.0f, 1.0f);
        float v = Clamp((wz-worldMinZ)/(worldMaxZ-worldMinZ), 0.0f, 1.0f);
        float fx = u*(resX-1), fy = v*(resZ-1);
        int x0=(int)fx, y0=(int)fy, x1=(x0+1)%resX, y1=(y0+1)%resZ;
        float dx=fx-x0, dy=fy-y0;
        return (heightGrid[y0][x0]*(1-dx)+heightGrid[y0][x1]*dx)*(1-dy)
             + (heightGrid[y1][x0]*(1-dx)+heightGrid[y1][x1]*dx)*dy;
    }
};

// ---------------------------------------------------------------------------
int main() {
    const int W = 1280, H = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(W, H, "魔法碎片：暗蚀纪元");

    Camera3D cam = {0};
    cam.up = {0,1,0}; cam.fovy = 60; cam.projection = CAMERA_PERSPECTIVE;
    float camDist = 16, camAngleX = 0, camAngleY = 25;

    // ---- 加载地形 ----
    Model terrainModel = {0};
    TerrainCollision coll;
    bool mapLoaded = false;
    float mapScale = 1.0f;
    if (FileExists("assets/models/" MAP_FILE) || FileExists("../assets/models/" MAP_FILE)) {
        const char *path = FileExists("assets/models/" MAP_FILE) ? "assets/models/" MAP_FILE : "../assets/models/" MAP_FILE;
        terrainModel = LoadModel(path);
        if (terrainModel.meshCount > 0) {
            BoundingBox bb = GetModelBoundingBox(terrainModel);
            if (bb.max.x - bb.min.x < 5 || bb.max.z - bb.min.z < 5) {
                mapScale = 10.0f;
                terrainModel.transform = MatrixScale(mapScale, mapScale, mapScale);
            }
            coll.BuildFromModel(terrainModel, 400);
            mapLoaded = true;
            TraceLog(LOG_INFO, "Map loaded: %d meshes, scale=%.1f", terrainModel.meshCount, mapScale);
        }
    }

    // ---- 玩家 ----
    Vector3 pPos = {0,0,0}, pVel = {0,0,0};
    bool grounded = true, crouching = false, blocking = false, sprint = false;
    float pH = 1.0f;
    if (mapLoaded) {
        float cx = (coll.worldMinX + coll.worldMaxX) * 0.5f;
        float cz = (coll.worldMinZ + coll.worldMaxZ) * 0.5f;
        pPos = {cx * mapScale, coll.GetHeight(cx, cz) * mapScale, cz * mapScale};
    }
    const float GRAVITY = -25, JUMP = 9, BASE_SPEED = 6;
    float speed = BASE_SPEED;

    // ---- 角色模型 ----
    Model standModel = {0}, walkModel = {0}, runModel = {0};
    Model *activeModel = 0;
    ModelAnimation *standAnims=0, *walkAnims=0, *runAnims=0;
    int standCount=0, walkCount=0, runCount=0, animFrame = 0, animMaxFrames = 1;
    bool hasChar = false;
    int standStart = 0, standEnd = 0;
    int walkStart = 0, walkEnd = 0;
    int runStart = 0, runEnd = 0;
    if (FileExists("../assets/models/character/stand.glb")) {
        const char *b = "../assets/models/character";
        standModel = LoadModelAssimp(TextFormat("%s/stand.glb", b));
        walkModel  = LoadModelAssimp(TextFormat("%s/walk.glb", b));
        runModel   = LoadModelAssimp(TextFormat("%s/run.glb", b));
        standAnims = LoadModelAnimationsAssimp(TextFormat("%s/stand.glb", b), &standCount);
        walkAnims  = LoadModelAnimationsAssimp(TextFormat("%s/walk.glb", b), &walkCount);
        runAnims   = LoadModelAnimationsAssimp(TextFormat("%s/run.glb", b), &runCount);
        activeModel = &standModel;
        hasChar = true;
        // clamp frame ranges to actual keyframeCount
        if (standCount > 0) standEnd = standAnims[0].keyframeCount - 1;
        if (walkCount > 0)  walkEnd  = walkAnims[0].keyframeCount - 1;
        if (runCount > 0)   runEnd   = runAnims[0].keyframeCount - 1;
    }
        Matrix *standMat = standModel.boneMatrices, *walkMat = walkModel.boneMatrices, *runMat = runModel.boneMatrices;

    // ---- 攻击 ----
    float atkTimer = 0, atkCooldown = 0.35f, swingAnim = 0;
    bool hitThisAttack = false;


    // ---- 字体 ----
    Font cnFont = {0};
    if (FileExists("C:/Windows/Fonts/simhei.ttf")) {
        const char *gly = "地面高度水面移动跳跃冲刺蹲下攻击格挡点击画面开始游戏"
            "当前区域浅滩草地密林山地石峰雪顶魔法碎片暗蚀纪元"
            "0123456789.:[]%/ -WASDCTRLSHIFTspace!?！？";
        int cnt = GetCodepointCount(gly), *codes = (int*)malloc(cnt * sizeof(int));
        const char *p2 = gly;
        for (int i = 0; i < cnt; i++) { int s = 0; codes[i] = GetCodepointNext(p2, &s); p2 += s; }
        cnFont = LoadFontEx("C:/Windows/Fonts/simhei.ttf", 64, codes, cnt);
        free(codes);
        if (cnFont.texture.id > 0) SetTextureFilter(cnFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    // ---- 树木/岩石 ----
    std::vector<Vector3> decor; // placeholder for trees (just positions)
    if (mapLoaded) for (int i = 0; i < 50; i++) {
        float x = GetRandomValue(-200, 200) / 10.0f * mapScale;
        float z = GetRandomValue(-200, 200) / 10.0f * mapScale;
        if (fabsf(x) < 3 || fabsf(z) < 3) continue;
        decor.push_back({x, 0, z});
    }

    DisableCursor();
    bool captured = true;
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_ESCAPE) && captured) { EnableCursor(); captured = false; }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !captured) { DisableCursor(); captured = true; }

        // ---- 视角 / 移动 ----
        if (captured) {
            Vector2 md = GetMouseDelta();
            camAngleX -= md.x * 0.2f;
            camAngleY += md.y * 0.2f;
            camAngleY = Clamp(camAngleY, -30, 85);
            camDist = Clamp(camDist - GetMouseWheelMove() * 2, 3, 28);
        }

        float yaw = camAngleX * DEG2RAD;
        Vector3 fwd = {-sinf(yaw), 0, -cosf(yaw)}, right = {cosf(yaw), 0, -sinf(yaw)}, move = {0,0,0};
        if (IsKeyDown(KEY_W)) { move.x += fwd.x; move.z += fwd.z; }
        if (IsKeyDown(KEY_S)) { move.x -= fwd.x; move.z -= fwd.z; }
        if (IsKeyDown(KEY_A)) { move.x -= right.x; move.z -= right.z; }
        if (IsKeyDown(KEY_D)) { move.x += right.x; move.z += right.z; }
        float moveLen = sqrtf(move.x*move.x + move.z*move.z);
        if (moveLen > 0) { move.x /= moveLen; move.z /= moveLen; }

        sprint = IsKeyDown(KEY_LEFT_SHIFT) && !crouching && grounded;
        crouching = IsKeyDown(KEY_LEFT_CONTROL);
        blocking = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && captured;
        pH = crouching ? 0.5f : 1.0f;
        speed = BASE_SPEED;
        if (sprint) speed = 10;
        if (crouching) speed = 3;
        if (blocking) speed = 2;

        // ---- 物理 ----
        if (captured && mapLoaded) {
            pPos.x += move.x * speed * dt;
            pPos.z += move.z * speed * dt;
            pPos.x = Clamp(pPos.x, (coll.worldMinX + 0.02f) * mapScale, (coll.worldMaxX - 0.02f) * mapScale);
            pPos.z = Clamp(pPos.z, (coll.worldMinZ + 0.02f) * mapScale, (coll.worldMaxZ - 0.02f) * mapScale);
            float gnd = coll.GetHeight(pPos.x / mapScale, pPos.z / mapScale) * mapScale;
            if (IsKeyPressed(KEY_SPACE) && grounded && !crouching) { pVel.y = JUMP; grounded = false; }
            pVel.y += GRAVITY * dt;
            pPos.y += pVel.y * dt;
            if (pPos.y <= gnd) { pPos.y = gnd; pVel.y = 0; grounded = true; }
            else grounded = false;
        }

        // ---- 攻击 ----
        if (captured && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && atkTimer <= 0 && !blocking) {
            atkTimer = atkCooldown; swingAnim = 1.0f; hitThisAttack = false;
        }
        if (atkTimer > 0) atkTimer -= dt;
        if (swingAnim > 0) swingAnim -= dt * 4;

        // ---- 动画 ----
        if (hasChar) {
            ModelAnimation *cur = 0; int maxF = 0; int startFrame = 0;
            if (sprint && runCount > 0)       { cur = &runAnims[0];  startFrame = runStart; animMaxFrames = runEnd; activeModel = &runModel; }
            else if (moveLen > 0 && walkCount > 0) { cur = &walkAnims[0]; startFrame = walkStart; animMaxFrames = walkEnd; activeModel = &walkModel; }
            else if (standCount > 0)          { cur = &standAnims[0]; startFrame = standStart; animMaxFrames = standEnd; activeModel = &standModel; }
            if (cur && animMaxFrames > startFrame) {
                animFrame = startFrame + ((animFrame - startFrame + 1) % (animMaxFrames - startFrame + 1));
                if (animFrame < startFrame) animFrame = startFrame;
                activeModel->boneMatrices = (activeModel==&standModel?standMat:activeModel==&walkModel?walkMat:runMat);
                UpdateModelAnimation(*activeModel, *cur, animFrame);
                activeModel->boneMatrices = NULL;
            }
        }

        // ---- 相机 ----
        float pitch = camAngleY * DEG2RAD, yawR = camAngleX * DEG2RAD;
        float eyeY = pPos.y + pH * 0.65f;
        cam.position = {pPos.x + camDist * cosf(pitch) * sinf(yawR), pPos.y + camDist * sinf(pitch) + pH * 0.4f, pPos.z + camDist * cosf(pitch) * cosf(yawR)};
        cam.target = {pPos.x, eyeY, pPos.z};

        // ---- 渲染 ----
        BeginDrawing();
        ClearBackground({135, 206, 235, 255});

        if (!mapLoaded) {
            DrawTextEx(cnFont, "地图加载失败", {20, 20}, 24, 1, RED);
        } else {
            BeginMode3D(cam);
            DrawModel(terrainModel, {0,0,0}, 1, WHITE);

            // 角色
            if (hasChar) {
                float footOff = 1.847f * pH;
                Vector3 charPos = {pPos.x, pPos.y + footOff, pPos.z};
                // 平滑全向转向
                float targetAng = (moveLen > 0 ? atan2f(move.x, move.z) : atan2f(fwd.x, fwd.z));
                static float smoothAng = 0;
                float diff = targetAng - smoothAng;
                while (diff > PI) diff -= 2*PI;
                while (diff < -PI) diff += 2*PI;
                smoothAng += diff * dt * 12.0f;
                float faceAngle = smoothAng * RAD2DEG - 90.0f;
                DrawModelEx(*activeModel, charPos, {0,1,0}, faceAngle, {pH*0.075f,pH*0.075f,pH*0.075f}, WHITE);
            } else {
                DrawCube({pPos.x, pPos.y + pH/2, pPos.z}, 0.55f, pH, 0.55f, {220,50,50,255});
            }
            EndMode3D();

            if (!captured) {
                const char *msg = "点击画面开始游戏";
                Vector2 sz = MeasureTextEx(cnFont, msg, 28, 1);
                DrawTextEx(cnFont, msg, {W/2 - sz.x/2, H/2 - sz.y/2}, 28, 1, WHITE);
            } else {
                DrawFPS(20, H - 50);
                DrawTextEx(cnFont, TextFormat("Anim: %d/%d", animFrame, animMaxFrames), {20, H-75}, 15, 1, Fade(WHITE,0.6f));
                DrawTextEx(cnFont, "[WASD]移动 [Shift]冲刺 [Ctrl]蹲下 [左键]攻击 [右键]格挡 [滚轮]缩放", {W/2 - 400, H - 28}, 15, 1, Fade(WHITE,0.6f));
            }
        }
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
    if (mapLoaded) UnloadModel(terrainModel);
    if (cnFont.texture.id > 0) UnloadFont(cnFont);
    CloseWindow();
    return 0;
}