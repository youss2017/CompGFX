#pragma once

#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "vulkan-1.lib")

extern "C" 
#ifdef BUILD_GRAPHICS_DLL
#if BUILD_GRAPHICS_DLL == 1
_declspec(dllexport)
#else
_declspec(dllimport)
#endif
#endif
void GraphicsEngine_Initalize();
extern "C"
#ifdef BUILD_GRAPHICS_DLL
#if BUILD_GRAPHICS_DLL == 1
_declspec(dllexport)
#else
_declspec(dllimport)
#endif
#endif
void GraphicsEngine_Destroy();
