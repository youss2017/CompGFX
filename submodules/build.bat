@echo off
echo "Make sure to run this using Visual Studio Developer Tools command prompt for msbuild"
echo "This tool requires cmake to be installed and reference inside %%PATH%% variable"
pause
echo "Building Assimp"
cd assimp
cmake . -DBUILD_SHARED_LIBS:BOOL="0"
msbuild Assimp.sln /p:Configuration=Debug -m
msbuild Assimp.sln /p:Configuration=Release -m
xcopy lib\Debug\* ..\..\library\Debug\ /K /D /H /Y
xcopy lib\Release\* ..\..\library\Release\ /K /D /H /Y
xcopy contrib\zlib\Debug\* ..\..\library\Debug\ /K /D /H /Y
xcopy contrib\zlib\Release\* ..\..\library\Release\ /K /D /H /Y
xcopy include\* ..\..\include\ /K /D /H /Y /s /e
cd ..
echo "Building glfw"
cd glfw
cmake . -DGLFW_BUILD_DOCS:BOOL="0" -DGLFW_BUILD_TESTS:BOOL="0" -DGLFW_INSTALL:BOOL="0" -DGLFW_BUILD_EXAMPLES:BOOL="0" -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release;"
msbuild GLFW.sln /p:Configuration=Debug -m
msbuild GLFW.sln /p:Configuration=Release -m
xcopy src\Debug\* ..\..\library\Debug\ /K /D /H /Y
xcopy src\Release\* ..\..\library\Release\ /K /D /H /Y
xcopy include\* ..\..\include\ /K /D /H /Y /s /e
cd ..
echo "Building Generic"
xcopy Utility\*.hpp ..\include\Utility\ /K /D /H /Y /s /e
xcopy glm\glm\* ..\include\glm\ /K /D /H /Y /s /e
xcopy stb\*.h ..\include\stb\ /K /D /H /Y /s /e
..\premake5.exe vs2022
cd Generic
REM /t:Clean;Rebuild
msbuild GenericWorkspace.sln /p:Configuration=Debug -m
msbuild GenericWorkspace.sln /p:Configuration=Release -m
cd ..
echo "Copying ImGui headers"
xcopy imgui\*.h ..\include\imgui\ /K /D /H /Y /s /e
xcopy imgui\backends\imgui_impl_vulkan.h ..\include\imgui\ /K /D /H /Y /s /e
xcopy imgui\backends\imgui_impl_glfw.h ..\include\imgui\ /K /D /H /Y /s /e
echo "Copying Vulkan Memory Allocator Headers"
xcopy VulkanMemoryAllocator\include\ ..\include\vma\ /K /D /H /Y /s /e
echo "Done"