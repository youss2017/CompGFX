#include "Entry.hpp"

#include <GLFW/glfw3.h>

#ifdef BUILD_GRAPHICS_DLL
extern "C" _declspec(dllexport) void GraphicsEngine_Initalize() {
	glfwInit();
}

extern "C" _declspec(dllexport) void GraphicsEngine_Destroy() {
	glfwTerminate();
}

#endif