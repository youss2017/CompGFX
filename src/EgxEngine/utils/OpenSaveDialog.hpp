#pragma once
#include <string>
#include <vector>

#ifdef BUILD_GRAPHICS_DLL
#if BUILD_GRAPHICS_DLL == 1
#define EGX_API _declspec(dllexport)
#else
#define EGX_API _declspec(dllimport)
#endif
#else
#define EGX_API
#endif

namespace Utils {

	std::string EGX_API CreateDialogFilter(std::vector<std::pair<std::string, std::string>> items);
	std::string EGX_API SaveAsDialog(const std::string& filter, int& OutChoosenFilterIndex);

}
