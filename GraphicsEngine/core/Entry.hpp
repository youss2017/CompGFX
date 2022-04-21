#pragma once

#pragma comment(lib, "glfw3dll.lib")

#ifdef BUILD_GRAPHICS_DLL
extern "C" _declspec(dllexport) void GraphicsEngine_Initalize();
extern "C" _declspec(dllexport) void GraphicsEngine_Destroy();
#else
extern "C" _declspec(dllimport) void GraphicsEngine_Initalize();
extern "C" _declspec(dllimport) void GraphicsEngine_Destroy();
#endif