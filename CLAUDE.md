# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# Configure (first time or when CMakeLists.txt changes)
cd build && cmake .. -G "MinGW Makefiles"

# Build all targets in parallel
cmake --build . -j4

# Game executables (run from build/ directory)
./map1.exe         # Chapter 1: 银风森林
./map2.exe         # Chapter 2
./char_test.exe    # Standalone character viewer (no map)
./glb_check.exe    # GLB file diagnostic tool
```

## Architecture

### Code Structure

| File | Purpose |
|------|---------|
| `src/main.cpp` | Game loop, input, physics, camera, rendering (~290 lines) |
| `src/assimp_loader.cpp` | Custom GLB model/animation loader via Assimp SDK (~520 lines) |
| `src/assimp_loader.h` | Public API for loader + FixAnimationPose |
| `src/char_test.cpp` | Standalone character test (no map) |
| `src/glb_check.cpp` | GLB diagnostic tool (prints scene graph, bone data, animation channels) |
| `CMakeLists.txt` | Dual-target build (map1/map2) + test tools |

### Build System

- CMake 3.14+, C++17, MinGW-w64 8.1.0
- Dependencies auto-fetched via FetchContent: raylib 6.0 (GitHub) + Assimp 5.4.3 (Gitee mirror)
- Two game executables from same source, differentiated by `MAP_FILE` compile definition

## Assimp Loader Key Design Decisions

### Bone Collection

Bones are collected from ALL meshes, sorted by bone count descending (mesh with most bones first). This ensures the authoritative mesh's `mOffsetMatrix` is used for bones shared across multiple meshes. The `collectMeshes[]` hardcoded array approach was replaced by runtime sorting.

### Bind Pose

- Extracted from `mOffsetMatrix.Inverse()` → model-space bind transform
- **Scale normalization**: common scale factor (e.g., 0.0165) is divided out from all bindPoses to prevent `inverse(bindPose) * currentPose` from producing extreme scales (60x+)
- Stored in `model.skeleton.bindPose[]`

### Animation Keyframes

- Loaded from Assimp animation channels, stored as LOCAL transforms
- **Root node fix**: root bone (parent=-1) keyframes are multiplied by root node transform before `ConvertPoseToModelSpace()`
- **ConvertPoseToModelSpace()**: walks parent chain to convert local → model-space (multi-pass for non-topological order)
- **FixAnimationPose()**: copies bindPose translation+scale to keyframePoses (GLB files often have rotation-only animation with zero position keys)

### Skeleton Animation Pipeline

```
UpdateModelAnimation(model, anim, frame):
  currentPose = lerp(keyframePoses[frame], keyframePoses[frame+1])
  boneMatrices[i] = MatrixInvert(bindPose[i]) * currentPose[i]
  → CPU skinning: animVertices = boneMatrices * originalVertices
  → Updates VBOs for rendering

// After UpdateModelAnimation:
activeModel->boneMatrices = NULL  // Prevents GPU skinning double-transform
```

### GLB Animation Data (character files)

All three character GLB files (stand/walk/run) have:
- **No position animation**: all position keys are `(0,0,0)` for every channel
- **Rotation-only**: 48/26/18 rotation keys per file, varying by bone
- **Scale keys**: single key at `(1,1,1)` for all channels
- This means FixAnimationPose is required to give bones their bind-pose positions

## Debug Tools

```bash
./glb_check.exe    # Checks scene graph, bone sharing, animation channels
./char_test.exe    # 3D viewer with 1/2/3 keys to switch animations
```

## Known Issues (2026-05-18)

- `b_root_0_060` bone has extreme bind pose translation `(-5.96, -64.38, -22.47)` in mesh 0 (weapon chain). Being at the leaf of the scene graph, it doesn't affect character bones.
- Character GLB files from FBX export have all position keys at `(0,0,0)` — rotation-only animation. `FixAnimationPose` compensates.
- The root node has scale `0.01` (FBX→GLB unit conversion). This is normalized out and baked into bindPose during model loading.

## Versioning

- Only increment minor version (v0.x) for functional changes.
- Never change major version unless explicitly told.
- Log changes in `log.md`.
