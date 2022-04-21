#pragma once
#include <string>

#ifndef GRAPHICS_API
#ifdef BUILD_GRAPHICS_DLL
#define GRAPHICS_API _declspec(dllexport)
#else
#define GRAPHICS_API _declspec(dllimport)
#endif
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