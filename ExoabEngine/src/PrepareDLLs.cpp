#include <Windows.h>

#pragma comment(lib, "delayimp")

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "assimp-vc143-mt.lib")

#if 0
#pragma comment(lib, "PhysXCharacterKinematic_static_64.lib")
#pragma comment(lib, "PhysXCommon_static_64.lib")
#pragma comment(lib, "PhysXCooking_static_64.lib")
#pragma comment(lib, "PhysXExtensions_static_64.lib")
#pragma comment(lib, "PhysXFoundation_static_64.lib")
#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "PhysXVehicle_static_64.lib")
#pragma comment(lib, "PhysX_static_64.lib")
#endif

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
	typedef void* (__stdcall AddDLL)(PCWSTR path);
	AddDLL* addDll = (AddDLL*)GetProcAddress(kernel, "AddDllDirectory");
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	addDll(L"D:\\GFX Apis\\ExoabEngine\\bin\\dep\\");
	int d = 0;
}