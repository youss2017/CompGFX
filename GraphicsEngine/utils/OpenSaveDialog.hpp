#pragma once
#include <string>
#include <vector>

#ifndef GRAPHICS_API
#ifdef BUILD_GRAPHICS_DLL
#define GRAPHICS_API _declspec(dllexport)
#else
#define GRAPHICS_API _declspec(dllimport)
#endif
#endif

namespace Utils {

	std::string GRAPHICS_API CreateDialogFilter(std::vector<std::pair<std::string, std::string>> items);
	std::string GRAPHICS_API SaveAsDialog(const std::string& filter, int& OutChoosenFilterIndex);

}
