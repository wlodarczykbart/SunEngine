Steps To Build Correctly
-Download assimp source from github
-Open CMake tool and input the CMakeLists.txt in root assimp directory
-Uncheck ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT becasue it causes conflicts with some image library used in SunEngine(Fix this in the future?)
-Uncheck BUILD_SHARED_LIBS because static libraries are expected
-Generate the visual studio solution
-Open visual studio solution and build Debug and Release
-Go into assimp build directory and copy the "include" and "lib" directories to the "build" directory where this ReadMe file exists