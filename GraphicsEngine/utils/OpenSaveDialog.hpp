#pragma once
#include <string>
#include <vector>

#ifdef BUILD_GRAPHICS_DLL
#if BUILD_GRAPHICS_DLL == 1
#define GRAPHICS_API _declspec(dllexport)
#else
#define GRAPHICS_API _declspec(dllimport)
#endif
#else
#define GRAPHICS_API
#endif

namespace Utils {

	std::string GRAPHICS_API CreateDialogFilter(std::vector<std::pair<std::string, std::string>> items);
	std::string GRAPHICS_API SaveAsDialog(const std::string& filter, int& OutChoosenFilterIndex);

}
