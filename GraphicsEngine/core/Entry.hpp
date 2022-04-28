#pragma once

#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "vulkan-1.lib")

#ifndef _DEBUG
#pragma comment(lib, "shaderc.lib")
#pragma comment(lib, "shaderc_combined.lib")
#pragma comment(lib, "shaderc_shared.lib")
#pragma comment(lib, "shaderc_util.lib")
#pragma comment(lib, "spirv-cross-core.lib")
#pragma comment(lib, "spirv-cross-cpp.lib")
#pragma comment(lib, "spirv-cross-glsl.lib")
#else
#pragma comment(lib, "shadercd.lib")
#pragma comment(lib, "shaderc_combinedd.lib")
#pragma comment(lib, "shaderc_sharedd.lib")
#pragma comment(lib, "shaderc_utild.lib")
#pragma comment(lib, "spirv-cross-cored.lib")
#pragma comment(lib, "spirv-cross-cppd.lib")
#pragma comment(lib, "spirv-cross-glsld.lib")
#endif


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
