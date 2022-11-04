@echo off
git clone https://github.com/youss2017/Utility
git clone https://github.com/ocornut/imgui
echo Switching to docking branch of ImGui
cd imgui
git switch -C docking
cd ..
echo Done