add_executable(FBXEditor
FBXEditor.h
FBXEditor.cpp
FBXEditorGUI.h
FBXEditorGUI.cpp
main.cpp
)

set_target_properties(FBXEditor PROPERTIES 
    VS_DEBUGGER_COMMAND_ARGUMENTS "-config${CMAKE_SOURCE_DIR}/FBXEditor/Settings.ini")

target_include_directories(FBXEditor PUBLIC 
"${CMAKE_SOURCE_DIR}/GameEngine"
"${CMAKE_SOURCE_DIR}/Editor"
"${CMAKE_SOURCE_DIR}/External/spdlog/include"
"${CMAKE_SOURCE_DIR}/External/ModelImporter"
"${CMAKE_SOURCE_DIR}/External/imgui/${IMGUI_VER}"
"${FBX_INCLUDE_DIR}")

target_link_libraries(FBXEditor optimized ${FBX_LIBRARY} debug ${FBX_LIBRARY_DEBUG})
target_link_libraries(FBXEditor GameEngine Editor ModelImporter)