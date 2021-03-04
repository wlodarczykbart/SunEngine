add_library(Editor
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imgui_internal.h
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imstb_rectpack.h
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imstb_textedit.h
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imstb_truetype.h
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imgui.cpp
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imgui_demo.cpp
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imgui_draw.cpp
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imgui_widgets.cpp
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imgui_tables.cpp
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imconfig.h
${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}/imgui.h
Editor.h
Editor.cpp
GUIRenderer.h
GUIRenderer.cpp
View.h
View.cpp
)

target_include_directories(Editor PUBLIC 
"${CMAKE_SOURCE_DIR}/RenderEngine"
"${CMAKE_SOURCE_DIR}/External/glm"
"${CMAKE_SOURCE_DIR}/External/spdlog/include"
"${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}")

target_link_libraries(Editor EngineTools RenderEngine spdlog d3d11.lib d3d12.lib dxgi.lib d3dcompiler.lib)