@echo off
git clone https://github.com/youss2017/Utility
git clone https://github.com/ocornut/imgui
git clone https://github.com/nothings/stb
git clone https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
git clone https://github.com/g-truc/glm
git clone https://github.com/assimp/assimp
git clone https://github.com/glfw/glfw
git clone https://github.com/nlohmann/json
echo Switching to docking branch of ImGui
cd imgui
git checkout docking
git branch --set-upstream-to=origin/docking docking
git pull
cd ..
echo Updating
cd Utility
git pull
cd ..
cd imgui
git pull
cd ..
cd stb
git pull
cd ..
cd VulkanMemoryAllocator
git pull
cd ..
cd glm
git pull
cd ..
cd assimp
git pull
cd ..
cd glfw
git pull
cd ..
cd json
git pull
cd ..
echo Done