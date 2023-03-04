@echo off
echo Remove non git files
cd assimp
git clean -d -f -x
cd ..
cd glfw
git clean -d -f -x
cd ..