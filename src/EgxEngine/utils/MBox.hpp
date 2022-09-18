#pragma once
#include <string>

#undef EGX_API
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

    enum MBox {
        INFO, WARNING, ERR
    };

    bool EGX_API Message(const char* title, const char* text, MBox type);
    bool EGX_API Message(const std::string& title, const std::string& text, MBox type);
    bool EGX_API Message(const wchar_t* title, const wchar_t* text, MBox type);
    bool EGX_API Message(const std::wstring& title, const std::wstring& text, MBox type);

};