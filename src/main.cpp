// =============================================================
// 魔法碎片：暗蚀纪元
// 引擎：raylib 6.0 | 语言：C++17
// =============================================================
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
        float totalSpanX = worldMaxX - worldMinX;
        float totalSpanZ = worldMaxZ - worldMinZ;

        struct B2 { float minX, maxX, minZ, maxZ; };
        std::vector<B2> mb(model.meshCount, {1e9f,-1e9f,1e9f,-1e9f});
        for (int m = 0; m < model.meshCount; m++) {
            Mesh *mesh = &model.meshes[m];
            if (!mesh->vertices || mesh->vertexCount == 0) continue;
            for (int v = 0; v < mesh->vertexCount; v++) {
                Vector3 p = Vector3Transform({mesh->vertices[v*3],mesh->vertices[v*3+1],mesh->vertices[v*3+2]}, mat);
                if (p.x < mb[m].minX) mb[m].minX = p.x;
                if (p.x > mb[m].maxX) mb[m].maxX = p.x;
                if (p.z < mb[m].minZ) mb[m].minZ = p.z;
                if (p.z > mb[m].maxZ) mb[m].maxZ = p.z;
            }
        }

        heightGrid.assign(resZ, std::vector<float>(resX, -1e9f));
        for (int m = 0; m < model.meshCount; m++) {
            if (mb[m].minX > mb[m].maxX) continue;
            float sx = mb[m].maxX - mb[m].minX, sz = mb[m].maxZ - mb[m].minZ;
            if (sx < totalSpanX * 0.4f || sz < totalSpanZ * 0.4f) continue;
            Mesh *mesh = &model.meshes[m];
            for (int v = 0; v < mesh->vertexCount; v++) {
                Vector3 p = Vector3Transform({mesh->vertices[v*3],mesh->vertices[v*3+1],mesh->vertices[v*3+2]}, mat);
                int gx = Clamp((int)((p.x-worldMinX)/totalSpanX*resX), 0, resX-1);
                int gz = Clamp((int)((p.z-worldMinZ)/totalSpanZ*resZ), 0, resZ-1);
                if (p.y > heightGrid[gz][gx]) heightGrid[gz][gx] = p.y;
            }
        }
        for (int z = 0; z < resZ; z++) for (int x = 0; x < resX; x++) {
            if (heightGrid[z][x] < -1e8f) {
                float h = 0; int c = 0;
                for (int dz = -2; dz <= 2; dz++) for (int dx = -2; dx <= 2; dx++) {
                    int nx = x+dx, nz = z+dz;
                    if (nx>=0 && nx<resX && nz>=0 && nz<resZ && heightGrid[nz][nx] > -1e8f)
                        { h += heightGrid[nz][nx]; c++; }
                }
                heightGrid[z][x] = (c > 0) ? h/c : 0;
            }
        }
    }

    float GetHeight(float wx, float wz) const {
        float u = Clamp((wx-worldMinX)/(worldMaxX-worldMinX), 0.0f, 1.0f);
        float v = Clamp((wz-worldMinZ)/(worldMaxZ-worldMinZ), 0.0f, 1.0f);
        float fx = u*(resX-1), fy = v*(resZ-1);
        int x0=(int)fx, y0=(int)fy, x1=(x0+1)%resX, y1=(y0+1)%resZ;
        float dx=fx-x0, dy=fy-y0;
        float h00=heightGrid[y0][x0], h10=heightGrid[y0][x1];
        float h01=heightGrid[y1][x0], h11=heightGrid[y1][x1];
        return (h00*(1-dx)+h10*dx)*(1-dy) + (h01*(1-dx)+h11*dx)*dy;
    }
};

struct Tree { Vector3 pos; float h, crownR; };
struct Rock { Vector3 pos; float r; };

// ---------------------------------------------------------------------------
int main()
{
    const int W = 1280, H = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(W, H, "魔法碎片：暗蚀纪元");

    Camera3D cam = { 0 };
    cam.up = {0,1,0}; cam.fovy = 60; cam.projection = CAMERA_PERSPECTIVE;
    float camDist = 16, camAngleX = 0, camAngleY = 25;

    // ---- 地形 ----
    Model terrainModel = {0};
    TerrainCollision coll;
    bool mapLoaded = false;
    float mapScale = 1.0f;

    if (FileExists("assets/models/" MAP_FILE) || FileExists("../assets/models/" MAP_FILE)) {
        const char *p = FileExists("assets/models/" MAP_FILE) ? "assets/models/" MAP_FILE : "../assets/models/" MAP_FILE;
        terrainModel = LoadModel(p);
        if (terrainModel.meshCount > 0) {
            BoundingBox bb = GetModelBoundingBox(terrainModel);
            if (bb.max.x-bb.min.x < 5 || bb.max.z-bb.min.z < 5) {
                mapScale = 10.0f;
                terrainModel.transform = MatrixScale(mapScale,mapScale,mapScale);
            }
            coll.BuildFromModel(terrainModel, 400);
            mapLoaded = true;
            TraceLog(LOG_INFO,"Map: (%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f)",
                coll.worldMinX,0.0f,coll.worldMinZ,coll.worldMaxX,0.0f,coll.worldMaxZ);
        }
    }

    // ---- 角色 ----
    Vector3 pPos = {0,0,0}, pVel = {0,0,0};
    bool grounded = true, crouching = false;
    float pH = 1.0f;
    if (mapLoaded) {
        float cx = (coll.worldMinX+coll.worldMaxX)*0.5f;
        float cz = (coll.worldMinZ+coll.worldMaxZ)*0.5f;
        pPos = {cx*mapScale, coll.GetHeight(cx,cz)*mapScale, cz*mapScale};
    }

    const float GRAVITY = -25, JUMP = 9;
    float speed = 6;

    // ---- 角色模型 ----
    Model playerModel = {0};
    ModelAnimation *standAnims=0,*walkAnims=0,*runAnims=0;
    int standCount=0,walkCount=0,runCount=0;
    int animFrame = 0;
    bool hasChar = false;

    if (FileExists("../assets/models/character/stand.glb")) {
        const char *b = "../assets/models/character";
        playerModel = LoadModelAssimp(TextFormat("%s/stand.glb",b));
        standAnims = LoadModelAnimationsAssimp(TextFormat("%s/stand.glb",b),&standCount);
        walkAnims  = LoadModelAnimationsAssimp(TextFormat("%s/walk.glb",b),&walkCount);
        runAnims   = LoadModelAnimationsAssimp(TextFormat("%s/run.glb",b),&runCount);
        hasChar = true;
    }

    // ---- 景物 ----
    SetRandomSeed((unsigned)time(0));
    std::vector<Tree> trees;
    if (mapLoaded) for (int i=0;i<50;i++) {
        float x=GetRandomValue(-200,200)/10.0f*mapScale, z=GetRandomValue(-200,200)/10.0f*mapScale;
        if (fabsf(x)<3||fabsf(z)<3) continue;
        float y=coll.GetHeight(x/mapScale,z/mapScale)*mapScale;
        if (y<0) continue;
        trees.push_back({{x,y,z}, 2.5f+GetRandomValue(5,25)/10.0f, 0.8f+GetRandomValue(5,15)/10.0f});
    }
    std::vector<Rock> rocks;
    if (mapLoaded) for (int i=0;i<30;i++) {
        float x=GetRandomValue(-200,200)/10.0f*mapScale, z=GetRandomValue(-200,200)/10.0f*mapScale;
        float y=coll.GetHeight(x/mapScale,z/mapScale)*mapScale;
        if (y<0) continue;
        rocks.push_back({{x,y,z}, 0.15f+GetRandomValue(5,18)/100.0f});
    }

    // ---- 字体 ----
    Font cnFont = {0};
    if (FileExists("C:/Windows/Fonts/simhei.ttf")) {
        const char *gly = "地面高度水面移动跳跃冲刺蹲下攻击格挡"
            "当前区域浅滩草地密林山地石峰雪顶"
            "魔法碎片暗蚀纪元地图测试战斗系统"
            "点击画面开始游戏"
            "0123456789.:[]%/ -WASDCTRLSHIFTspace!?！？";
        int cnt=GetCodepointCount(gly), *codes=(int*)malloc(cnt*sizeof(int));
        const char *p=gly;
        for (int i=0;i<cnt;i++){int s=0;codes[i]=GetCodepointNext(p,&s);p+=s;}
        cnFont=LoadFontEx("C:/Windows/Fonts/simhei.ttf",64,codes,cnt);
        free(codes);
        if (cnFont.texture.id>0) SetTextureFilter(cnFont.texture,TEXTURE_FILTER_BILINEAR);
    }

    DisableCursor();
    bool captured = true;
    SetTargetFPS(60);

    // =====================================================================
    //                         主循环
    // =====================================================================
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_ESCAPE) && captured) { EnableCursor(); captured = false; }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !captured) { DisableCursor(); captured = true; }

        if (captured) {
            Vector2 md = GetMouseDelta();
            camAngleX -= md.x*0.2f;
            camAngleY += md.y*0.2f;
            camAngleY = Clamp(camAngleY, -30, 85);
            camDist = Clamp(camDist - GetMouseWheelMove()*2, 3, 28);
        }

        float yaw = camAngleX*DEG2RAD;
        Vector3 fwd = {-sinf(yaw),0,-cosf(yaw)};
        Vector3 right = {cosf(yaw),0,-sinf(yaw)};
        Vector3 move = {0,0,0};
        if (IsKeyDown(KEY_W)) {move.x+=fwd.x; move.z+=fwd.z;}
        if (IsKeyDown(KEY_S)) {move.x-=fwd.x; move.z-=fwd.z;}
        if (IsKeyDown(KEY_A)) {move.x-=right.x; move.z-=right.z;}
        if (IsKeyDown(KEY_D)) {move.x+=right.x; move.z+=right.z;}
        float moveLen = sqrtf(move.x*move.x+move.z*move.z);
        if (moveLen>0) {move.x/=moveLen; move.z/=moveLen;}

        bool sprint = IsKeyDown(KEY_LEFT_SHIFT) && !crouching && grounded;
        crouching = IsKeyDown(KEY_LEFT_CONTROL);
        pH = crouching ? 0.5f : 1.0f;

        speed = 6; if (sprint) speed=10; if (crouching) speed=3;

        // ---- 物理 ----
        if (captured && mapLoaded) {
            pPos.x += move.x*speed*dt; pPos.z += move.z*speed*dt;
            float edge = 0.02f;
            float xMin = (coll.worldMinX+edge)*mapScale, xMax=(coll.worldMaxX-edge)*mapScale;
            float zMin = (coll.worldMinZ+edge)*mapScale, zMax=(coll.worldMaxZ-edge)*mapScale;
            pPos.x = Clamp(pPos.x,xMin,xMax); pPos.z = Clamp(pPos.z,zMin,zMax);

            float gnd = coll.GetHeight(pPos.x/mapScale,pPos.z/mapScale)*mapScale;
            if (IsKeyPressed(KEY_SPACE) && grounded && !crouching)
                { pVel.y = JUMP; grounded = false; }
            pVel.y += GRAVITY*dt; pPos.y += pVel.y*dt;
            if (pPos.y <= gnd) { pPos.y = gnd; pVel.y = 0; grounded = true; }
            else grounded = false;
        }

        // ---- 动画 ----
        if (hasChar) {
            ModelAnimation *cur = 0; int maxF = 0;
            if (sprint && runCount>0)     { cur=&runAnims[0];  maxF=runAnims[0].keyframeCount; }
            else if (moveLen>0 && walkCount>0) { cur=&walkAnims[0]; maxF=walkAnims[0].keyframeCount; }
            else if (standCount>0)         { cur=&standAnims[0]; maxF=standAnims[0].keyframeCount; }
            if (cur && maxF>0) {
                animFrame = (animFrame+1)%maxF;
                UpdateModelAnimation(playerModel, *cur, animFrame);
            }
        }

        // ---- 相机 ----
        float pitch=camAngleY*DEG2RAD, yawR=camAngleX*DEG2RAD;
        float eyeY = pPos.y+pH*0.65f;
        cam.position.x = pPos.x+camDist*cosf(pitch)*sinf(yawR);
        cam.position.y = pPos.y+camDist*sinf(pitch)+pH*0.4f;
        cam.position.z = pPos.z+camDist*cosf(pitch)*cosf(yawR);
        cam.target = {pPos.x, eyeY, pPos.z};

        // ---- 渲染 ----
        BeginDrawing();
        ClearBackground({180,210,240,255});

        if (!mapLoaded) {
            const char *msg = "Map load failed";
            Vector2 sz = MeasureTextEx(cnFont,msg,20,1);
            DrawTextEx(cnFont,msg,{W/2-sz.x/2,H/2-10},20,1,RED);
        } else {
            BeginMode3D(cam);
            DrawModel(terrainModel,{0,0,0},1,WHITE);

            for (auto &t : trees) {
                DrawCylinder({t.pos.x,t.pos.y+t.h/2,t.pos.z},0.12f,0.18f,t.h,6,{120,80,45,255});
                float ch=t.crownR*1.5f;
                DrawCylinder({t.pos.x,t.pos.y+t.h+ch/2,t.pos.z},0,t.crownR,ch,6,{40,150,40,255});
            }
            for (auto &r : rocks) DrawSphere(r.pos,r.r,{130,120,110,255});

            // ---- 角色 ----
            if (hasChar) {
                // 脚底偏移 = 原始网格最低点(-1.436) × 骨骼缩放(1.286) = 1.847
                float footOff = 1.847f * pH;
                // 然而 model.transform 的缩放也影响位移，需要补偿
                // 计算思路：foot_local = -1.847 → 经 model.transform 缩放 → 希望落在 pPos.y
                float charY = pPos.y + footOff;
                Vector3 charPos = {pPos.x, charY, pPos.z};
                float faceAngle = (moveLen>0) ? atan2f(move.x,move.z) : atan2f(fwd.x,fwd.z);
                faceAngle = faceAngle*RAD2DEG;
                Vector3 sc = {pH,pH,pH};
                DrawModelEx(playerModel, charPos, {0,1,0}, faceAngle, sc, WHITE);
            } else {
                Vector3 pc = {pPos.x,pPos.y+pH/2,pPos.z};
                DrawCube(pc,0.55f,pH,0.55f,{220,50,50,255});
            }

            EndMode3D();

            if (!captured) {
                const char *msg = "点击画面开始游戏";
                Vector2 sz = MeasureTextEx(cnFont,msg,28,1);
                DrawTextEx(cnFont,msg,{W/2-sz.x/2,H/2-sz.y/2},28,1,WHITE);
            } else {
                DrawFPS(20,H-50);
                const char *h = "[WASD]移动 [Shift]冲刺 [Ctrl]蹲下  [滚轮]缩放";
                DrawTextEx(cnFont, h, {W/2-MeasureTextEx(cnFont,h,15,1).x/2, H-28}, 15, 1, Fade(WHITE,0.6f));
            }
        }
        EndDrawing();
    }

    // ---- 清理 ----
    if (hasChar) {
        UnloadModel(playerModel);
        if (standAnims) UnloadModelAnimations(standAnims,standCount);
        if (walkAnims)  UnloadModelAnimations(walkAnims,walkCount);
        if (runAnims)   UnloadModelAnimations(runAnims,runCount);
    }
    if (mapLoaded) UnloadModel(terrainModel);
    if (cnFont.texture.id>0) UnloadFont(cnFont);
    CloseWindow();
    return 0;
}
