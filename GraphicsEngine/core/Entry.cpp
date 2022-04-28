#include "Entry.hpp"

#include <GLFW/glfw3.h>

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
