add_executable(DXShaderCompiler
main.cpp
)

target_include_directories(DXShaderCompiler PUBLIC 
"${CMAKE_SOURCE_DIR}/EngineTools"
"${CMAKE_SOURCE_DIR}/Math"
"${CMAKE_SOURCE_DIR}/RenderEngine")

target_link_libraries(DXShaderCompiler EngineTools)
target_link_libraries(DXShaderCompiler RenderEngine)
target_link_libraries(DXShaderCompiler "d3d11.lib")