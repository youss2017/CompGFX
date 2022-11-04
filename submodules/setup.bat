@echo off
git clone https://github.com/youss2017/Utility
git clone https://github.com/ocornut/imgui
echo Switching to docking branch of ImGui
cd imgui
git checkout docking
git branch --set-upstream-to=origin/docking docking
git pull
cd ..
echo Done