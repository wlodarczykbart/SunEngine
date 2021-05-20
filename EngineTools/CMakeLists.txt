add_library(EngineTools
${CMAKE_SOURCE_DIR}/External/stb/stb_image.h
${CMAKE_SOURCE_DIR}/External/stb/stb_truetype.h
${CMAKE_SOURCE_DIR}/External/stb/stb_dxt.h
${CMAKE_SOURCE_DIR}/External/stb/stb_image_write.h
${CMAKE_SOURCE_DIR}/External/FastNoise/FastNoise.h
${CMAKE_SOURCE_DIR}/External/FastNoise/FastNoise.cpp
${CMAKE_SOURCE_DIR}/External/SOIL2/image_DXT.h
${CMAKE_SOURCE_DIR}/External/SOIL2/stbi_DDS.h
${CMAKE_SOURCE_DIR}/External/SOIL2/stbi_DDS_c.h
FileBase.h
FileReader.h
FileWriter.h
Image.h
MemBuffer.h
Pixel.h
Serializable.h
StreamBase.h
StreamReader.h
StreamWriter.h
StringUtil.h
Timer.h
Types.h
BufferBase.h
BufferReader.h
BufferWriter.h
ConfigFile.h
MipMapGenerator.h
TimeImpl.h
BufferBase.cpp
BufferReader.cpp
BufferWriter.cpp
ConfigFile.cpp
FileBase.cpp
FileReader.cpp
FileWriter.cpp
Image.cpp
MemBuffer.cpp
Serializable.cpp
StreamBase.cpp
StreamReader.cpp
StreamWriter.cpp
StringUtil.cpp
Timer.cpp
MipMapGenerator.cpp
TimeImpl.cpp
)

target_include_directories(EngineTools PUBLIC "${CMAKE_SOURCE_DIR}/External/stb/")
target_include_directories(EngineTools PUBLIC "${CMAKE_SOURCE_DIR}/External/FastNoise/")
target_include_directories(EngineTools PUBLIC "${CMAKE_SOURCE_DIR}/External/Urho3D/")
target_include_directories(EngineTools PUBLIC "${CMAKE_SOURCE_DIR}/External/SOIL2/")