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

// Load Everything (PhysX)
#pragma comment(lib, "LowLevelAABB_static_64.lib")
#pragma comment(lib, "LowLevelDynamics_static_64.lib")
#pragma comment(lib, "LowLevel_static_64.lib")
#pragma comment(lib, "PhysXCharacterKinematic_static_64.lib")
#pragma comment(lib, "PhysXCommon_64.lib")
#pragma comment(lib, "PhysXCooking_64.lib")
#pragma comment(lib, "PhysXExtensions_static_64.lib")
#pragma comment(lib, "PhysXFoundation_64.lib")
#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "PhysXTask_static_64.lib")
#pragma comment(lib, "PhysXVehicle_static_64.lib")
#pragma comment(lib, "PhysX_64.lib")
#pragma comment(lib, "SceneQuery_static_64.lib")
#pragma comment(lib, "SimulationController_static_64.lib") 

void PrepareDLLs()
{
#if defined(_DEBUG)
	SetDllDirectoryA("external\\bin\\debug");
#else
	SetDllDirectoryA("external\\bin\\release");
#endif
}