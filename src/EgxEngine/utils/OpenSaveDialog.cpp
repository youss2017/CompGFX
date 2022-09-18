#ifdef _WIN32
#include "OpenSaveDialog.hpp"
#include <Windows.h>
#ifdef WIN32_LEAN_AND_MEAN
#include <commdlg.h>
#endif
#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>

std::string EGX_API Utils::CreateDialogFilter(std::vector<std::pair<std::string, std::string>> items) {
	std::string filter = "";
	for (auto&[name, ft] : items) {
		filter += name + " (" + ft + ")" + "$" + ft + '$';
	}
	filter += "$$";
	char* temp = new char[filter.size()];
	for (int i = 0; i < filter.size(); i++) {
		if (filter[i] == '$')
			temp[i] = '\0';
		else
			temp[i] = filter[i];
	}
	std::string filter2;
	filter2.resize(filter.size());
	memcpy(filter2.data(), temp, filter.size());
	delete[] temp;
	return filter2;
}

std::string EGX_API Utils::SaveAsDialog(const std::string& filter, int& OutChoosenFilterIndex) {
	char szFile[256]{0};
	OPENFILENAMEA open{};
	open.lStructSize = sizeof(OPENFILENAMEA);
	open.hwndOwner = nullptr;// glfwGetWin32Window(Global::Window->GetWindow());
	open.lpstrFilter = filter.c_str();// "Source\0*.C;*.CXX\0All\0*.*\0txt\0*.txt\0";
	open.nFilterIndex = 0;
	open.nMaxFile = sizeof(szFile);
	open.lpstrFile = szFile;
	open.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetSaveFileNameA(&open) == TRUE) {
		std::string path = szFile;
		OutChoosenFilterIndex = open.nFilterIndex - 1;
		return path;
	}
	return std::string();
}

#else
#error "Not supported."
#endif

