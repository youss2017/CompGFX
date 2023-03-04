@echo off
echo "Make sure to run this using Visual Studio Developer Tools command prompt for msbuild"
echo "This tool requires cmake to be installed and reference inside %%PATH%% variable"
pause
echo "Building Assimp"
cd assimp
cmake . -DLIBRARY_SUFFIX:STRING="" -DCMAKE_DEBUG_POSTFIX:STRING="" -DBUILD_SHARED_LIBS:BOOL="1"
msbuild Assimp.sln /p:Configuration=Release -m
xcopy lib\Debug\* ..\..\lib\Debug\ /K /D /H /Y
xcopy lib\Release\* ..\..\lib\Release\ /K /D /H /Y
xcopy bin\Debug\* ..\..\lib\Debug\ /K /D /H /Y
xcopy bin\Release\* ..\..\lib\Debug\ /K /D /H /Y
xcopy contrib\zlib\Debug\* ..\..\lib\Debug\ /K /D /H /Y
xcopy contrib\zlib\Release\* ..\..\lib\Release\ /K /D /H /Y
xcopy include\* ..\..\include\ /K /D /H /Y /s /e
cd ..
echo "Building GLFW"
cd glfw
cmake . -DGLFW_BUILD_DOCS:BOOL="0" -DGLFW_BUILD_TESTS:BOOL="0" -DGLFW_INSTALL:BOOL="0" -DGLFW_BUILD_EXAMPLES:BOOL="0" -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release;"
msbuild GLFW.sln /p:Configuration=Debug -m
msbuild GLFW.sln /p:Configuration=Release -m
xcopy src\Debug\* ..\..\lib\Debug\ /K /D /H /Y
xcopy src\Release\* ..\..\lib\Release\ /K /D /H /Y
xcopy include\* ..\..\include\ /K /D /H /Y /s /e
cd ..
echo "Compiling Single Header Dependencies"
..\premake5.exe vs2022
cd CompinedDependencies
msbuild Dependencies.sln /p:Configuration=Debug -m
msbuild Dependencies.sln /p:Configuration=Release -m
cd ..
echo "Copying Single Header Libs"
xcopy Utility\*.hpp ..\include\Utility\ /K /D /H /Y /s /e
xcopy glm\glm\* ..\include\glm\ /K /D /H /Y /s /e
xcopy stb\*.h ..\include\stb\ /K /D /H /Y /s /e
echo "Copying ImGui headers"
xcopy imgui\*.h ..\include\imgui\ /K /D /H /Y /s /e
xcopy imgui\backends\imgui_impl_vulkan.h ..\include\imgui\ /K /D /H /Y /s /e
xcopy imgui\backends\imgui_impl_glfw.h ..\include\imgui\ /K /D /H /Y /s /e
echo "Copying Vulkan Memory Allocator Headers"
xcopy VulkanMemoryAllocator\include\ ..\include\vma\ /K /D /H /Y /s /e
echo "Copying JSON"
xcopy json\single_include\nlohmann\ ..\include\json\ /K /D /H /Y /s /e
echo "Copying SQLite"
xcopy sqlite\ ..\include\sqlite\ /K /D /H /Y /s /e
echo "Done"