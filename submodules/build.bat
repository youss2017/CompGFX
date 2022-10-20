@echo off
echo "Make sure to run this using Visual Studio Developer Tools command prompt for msbuild"
echo "This tool requires cmake to be installed and reference inside %%PATH%% variable"
pause
echo "Building Assimp"
cd assimp
cmake . -DBUILD_SHARED_LIBS:BOOL="0"
msbuild Assimp.sln /p:Configuration=Debug
msbuild Assimp.sln /p:Configuration=Release
xcopy lib\Debug\* ..\..\library\Debug\ /K /D /H /Y
xcopy lib\Release\* ..\..\library\Release\ /K /D /H /Y
xcopy contrib\zlib\Debug\* ..\..\library\Debug\ /K /D /H /Y
xcopy contrib\zlib\Release\* ..\..\library\Release\ /K /D /H /Y
xcopy include\* ..\..\include\ /K /D /H /Y /s /e
cd ..
echo "Building glfw"
cd glfw
cmake . -DGLFW_BUILD_DOCS:BOOL="0" -DGLFW_BUILD_TESTS:BOOL="0" -DGLFW_INSTALL:BOOL="0" -DGLFW_BUILD_EXAMPLES:BOOL="0" -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release;"
msbuild GLFW.sln /p:Configuration=Debug
msbuild GLFW.sln /p:Configuration=Release
xcopy src\Debug\* ..\..\library\Debug\ /K /D /H /Y
xcopy src\Release\* ..\..\library\Release\ /K /D /H /Y
xcopy include\* ..\..\include\ /K /D /H /Y /s /e
cd ..
echo "Building Generic"
xcopy glm\glm\* ..\include\glm\ /K /D /H /Y /s /e
xcopy stb\*.h ..\include\stb\ /K /D /H /Y /s /e
xcopy Utilities\*.hpp ..\include\util\ /K /D /H /Y /s /e
xcopy Utilities\*.h ..\include\util\ /K /D /H /Y /s /e
..\premake5.exe vs2022
cd Generic
msbuild GenericWorkspace.sln /p:Configuration=Debug
msbuild GenericWorkspace.sln /p:Configuration=Release
cd ..
echo "Copying ImGui headers"
xcopy imgui\*.h ..\include\imgui\ /K /D /H /Y /s /e
xcopy imgui\backends\imgui_impl_vulkan.h ..\include\imgui\ /K /D /H /Y /s /e
xcopy imgui\backends\imgui_impl_glfw.h ..\include\imgui\ /K /D /H /Y /s /e
echo "Copying Vulkan Memory Allocator Headers"
xcopy VulkanMemoryAllocator\include\ ..\include\vma\ /K /D /H /Y /s /e
echo "Done"