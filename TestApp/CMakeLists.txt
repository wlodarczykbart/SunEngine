add_executable(TestApp
main.cpp
)

target_include_directories(TestApp PUBLIC 
"${CMAKE_SOURCE_DIR}/RenderEngine"
"${CMAKE_SOURCE_DIR}/External/glm"
"${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}")

target_link_libraries(TestApp EngineTools RenderEngine d3d11)