#include <Windows.h>

#pragma comment(lib, "delayimp")

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "shaderc_shared.lib")

#if defined(_DEBUG)
#pragma comment(lib, "spirv-cross-cored.lib")
#pragma comment(lib, "spirv-cross-cppd.lib")
#pragma comment(lib, "spirv-cross-glsld.lib")
#pragma comment(lib, "spirv-cross-glsld.lib")
#pragma comment(lib, "assimp-vc143-mtd.lib")
#else
#pragma comment(lib, "spirv-cross-core.lib")
#pragma comment(lib, "spirv-cross-cpp.lib")
#pragma comment(lib, "spirv-cross-glsl.lib")
#pragma comment(lib, "spirv-cross-glsl.lib")
#pragma comment(lib, "assimp-vc143-mt.lib")
#endif

void PrepareDLLs()
{
#if defined(_DEBUG)
	SetDllDirectoryA("external\\bin\\debug");
#else
	SetDllDirectoryA("external\\bin\\release");
#endif
}