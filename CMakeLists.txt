cmake_minimum_required(VERSION 3.14)
project(MagicShard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动从 GitHub 下载并编译 raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

# 自动下载 Assimp (从 Gitee 镜像)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://gitee.com/mirrors/assimp.git
    GIT_TAG v5.4.3
    GIT_SHALLOW TRUE
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas -Wno-error=unknown-pragmas")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas -Wno-error=unknown-pragmas")
FetchContent_MakeAvailable(assimp)

# 设置可执行文件输出到 build/ 目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 同一份源代码编译两个程序
add_executable(map1 src/main.cpp src/assimp_loader.cpp)
target_link_libraries(map1 raylib assimp)
target_include_directories(map1 PRIVATE src)
target_compile_definitions(map1 PRIVATE MAP_FILE="map_01_forest.glb")

add_executable(map2 src/main.cpp src/assimp_loader.cpp)
target_link_libraries(map2 raylib assimp)
target_include_directories(map2 PRIVATE src)
target_compile_definitions(map2 PRIVATE MAP_FILE="map_02.glb")

add_executable(char_test src/char_test.cpp src/assimp_loader.cpp)
target_link_libraries(char_test raylib assimp)
target_include_directories(char_test PRIVATE src)

add_executable(glb_check src/glb_check.cpp)
target_link_libraries(glb_check assimp)
target_include_directories(glb_check PRIVATE src)
