#pragma once
#include <string>

#undef GRAPHICS_API
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

    enum MBox {
        INFO, WARNING, ERR
    };

    bool GRAPHICS_API Message(const char* title, const char* text, MBox type);
    bool GRAPHICS_API Message(const std::string& title, const std::string& text, MBox type);
    bool GRAPHICS_API Message(const wchar_t* title, const wchar_t* text, MBox type);
    bool GRAPHICS_API Message(const std::wstring& title, const std::wstring& text, MBox type);

};