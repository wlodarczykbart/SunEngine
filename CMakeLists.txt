cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

# set the project name and version
project(SunEngine)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
cmake_policy(SET CMP0060 NEW)

#wd4267 - things like uint from size_t stl structures when not in x86 compiler
#wd4201 - nameless strucuts

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4267 /wd4201")

find_package(Vulkan REQUIRED)

set(IMGUI_VER imgui-1.80)
set(ZLIB_VER zlib-1.2.11)
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/lib/")

add_subdirectory(EngineTools)
add_subdirectory(RenderEngine)
add_subdirectory(External/ModelImporter)
add_subdirectory(External/spdlog)
add_subdirectory(GameEngine)
add_subdirectory(Editor)
add_subdirectory(GameEditor)
add_subdirectory(FBXEditor)
add_subdirectory(External/zlib/${ZLIB_VER})
add_subdirectory(TestApp)
add_subdirectory(VRApp)

add_dependencies(RenderEngine EngineTools)
add_dependencies(GameEngine RenderEngine)
add_dependencies(Editor spdlog RenderEngine)
add_dependencies(GameEditor Editor GameEngine ModelImporter)
add_dependencies(FBXEditor Editor GameEngine ModelImporter)