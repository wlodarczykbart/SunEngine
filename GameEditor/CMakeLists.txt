add_executable(GameEditor
GameEditor.h
GameEditor.cpp
DefaultShaders.h
DefaultShaders.cpp
GameEditorGUI.h
GameEditorGUI.cpp
SceneRenderer.h
SceneRenderer.cpp
GameEditorViews.h
GameEditorViews.cpp
main.cpp
)

set_target_properties(GameEditor PROPERTIES 
    VS_DEBUGGER_COMMAND_ARGUMENTS "-config${CMAKE_SOURCE_DIR}/GameEditor/Settings.ini")

target_include_directories(GameEditor PUBLIC 
"${CMAKE_SOURCE_DIR}/GameEngine"
"${CMAKE_SOURCE_DIR}/Editor"
"${CMAKE_SOURCE_DIR}/External/spdlog/include"
"${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}")

target_link_libraries(GameEditor GameEngine Editor ModelImporter zlib)
target_link_libraries(GameEditor optimized "${LIB_DIR}/Release/assimp-vc142-mt.lib" debug "${LIB_DIR}/Debug/assimp-vc142-mtd.lib")