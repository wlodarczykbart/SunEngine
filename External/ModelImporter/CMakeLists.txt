add_library(ModelImporter
ModelImporterMath.h
ModelImporterStr.h
OBJImporter.h
3DImporter.cpp
FBXImporter.cpp
OBJImporter.cpp
triangulate.cpp
3DImporter.h
FBXImporter.h
triangulate.h
)

include("FindFBX.cmake")

target_include_directories(ModelImporter PUBLIC "${FBX_INCLUDE_DIR}")

target_link_libraries(ModelImporter optimized ${FBX_LIBRARY} debug ${FBX_LIBRARY_DEBUG})
 