@echo off
echo Adding submodules
:: glm
echo Adding glm (1/7)
git submodule add https://github.com/g-truc/glm
:: VulkanMemoryAllocator
echo Adding Vulkan Memory Allocator (2/7)
git submodule add https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
:: Assimp
echo Adding Assimp (3/7)
git submodule add https://github.com/assimp/assimp
:: glfw
echo Adding glfw (4/7)
git submodule add https://github.com/glfw/glfw
:: stb
echo Adding stb (5/7)
git submodule add https://github.com/nothings/stb
:: utilites
echo Adding Utilites (6/7)
git submodule add https://github.com/youss2017/Utilities
:: imgui
echo Adding imgui docking branch (7/7)
git submodule add https://github.com/ocornut/imgui imgui
cd imgui
git switch -C docking
cd ..
echo Done