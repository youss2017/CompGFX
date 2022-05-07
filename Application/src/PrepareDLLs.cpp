#include <Windows.h>

#pragma comment(lib, "delayimp")

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "assimp-vc143-mt.lib")

#pragma comment(lib, "LowLevelAABB_static_64.lib")
#pragma comment(lib, "LowLevelDynamics_static_64.lib")
#pragma comment(lib, "LowLevel_static_64.lib")
#pragma comment(lib, "PhysXCharacterKinematic_static_64.lib")
#pragma comment(lib, "PhysXCommon_64.lib")
#pragma comment(lib, "PhysXCommon_static_64.lib")
#pragma comment(lib, "PhysXCooking_64.lib")
#pragma comment(lib, "PhysXCooking_static_64.lib")
#pragma comment(lib, "PhysXExtensions_static_64.lib")
#pragma comment(lib, "PhysXFoundation_64.lib")
#pragma comment(lib, "PhysXFoundation_static_64.lib")
#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "PhysXTask_static_64.lib")
#pragma comment(lib, "PhysXVehicle_static_64.lib")
#pragma comment(lib, "PhysX_64.lib")
#pragma comment(lib, "PhysX_static_64.lib")
#pragma comment(lib, "SceneQuery_static_64.lib")
#pragma comment(lib, "SimulationController_static_64.lib")

#pragma comment(lib, "GraphicsEngine.lib")
#pragma comment(lib, "Audio.lib")

/*
	***********************[WARNING]***********************
	!!! Make sure DLLs are in the same folder as the executable.
*/
#include <string>

void PrepareDLLs()
{
	// This is for Windows 7, as recommeded by MSDN.
	// Look at remarks at https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-adddlldirectory
	HMODULE kernel = LoadLibraryA("Kernel32.dll");
	using AddDLL = void* (__stdcall)(PCWSTR path);
	AddDLL* addDll = (AddDLL*)GetProcAddress(kernel, "AddDllDirectory");
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	addDll(L"D:\\GFX Apis\\ExoabEngine\\bin\\dep\\");
	addDll(L"D:\\GFX Apis\\ExoabEngine\\bin\\dep\\debug\\");
	int d = 0;
}