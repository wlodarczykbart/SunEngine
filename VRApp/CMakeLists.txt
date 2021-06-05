add_executable(VRApp
main.cpp
)

target_include_directories(VRApp PUBLIC 
"${CMAKE_SOURCE_DIR}/RenderEngine"
"${CMAKE_SOURCE_DIR}/GameEngine"
"${CMAKE_SOURCE_DIR}/External/glm")

target_link_libraries(VRApp EngineTools RenderEngine d3d11 dxgi Vulkan::Vulkan GameEngine)
target_link_libraries(VRApp optimized "${LIB_DIR}/Release/assimp-vc142-mt.lib" debug "${LIB_DIR}/Debug/assimp-vc142-mtd.lib")
target_link_libraries(VRApp optimized "${LIB_DIR}/Release/openxr_loader.lib" debug "${LIB_DIR}/Debug/openxr_loaderd.lib")