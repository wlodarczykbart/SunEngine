add_library(GameEngine
Game.h
Game.cpp
)

target_include_directories(GameEngine PUBLIC 
"${CMAKE_SOURCE_DIR}/EngineTools"
"${CMAKE_SOURCE_DIR}/RenderEngine")