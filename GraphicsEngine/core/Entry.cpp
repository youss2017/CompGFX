#include "Entry.hpp"

#include <GLFW/glfw3.h>
#include "Entry.hpp"

#pragma comment(lib, "shaderc_shared.lib")

#if defined(_DEBUG)
#pragma comment(lib, "spirv-cross-cored.lib")
#pragma comment(lib, "spirv-cross-cppd.lib")
#pragma comment(lib, "spirv-cross-glsld.lib")
#pragma comment(lib, "spirv-cross-glsld.lib")
#else
#pragma comment(lib, "spirv-cross-core.lib")
#pragma comment(lib, "spirv-cross-cpp.lib")
#pragma comment(lib, "spirv-cross-glsl.lib")
#pragma comment(lib, "spirv-cross-glsl.lib")
#endif

extern "C"
#ifdef BUILD_GRAPHICS_DLL
_declspec(dllexport)
#endif
void GraphicsEngine_Initalize() {
	glfwInit();
}

extern "C" 
#ifdef BUILD_GRAPHICS_DLL
_declspec(dllexport)
#endif
void GraphicsEngine_Destroy() {
	glfwTerminate();
}
