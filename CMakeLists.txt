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

add_subdirectory(EngineTools)
add_subdirectory(RenderEngine)
add_subdirectory(DXShaderCompiler)
add_subdirectory(External/ModelImporter)

add_dependencies(RenderEngine EngineTools)
add_dependencies(DXShaderCompiler RenderEngine)