#pragma once
#include <string>

namespace Utils {

    enum MBox {
        INFO, WARNING, ERR
    };

    bool Message(const char* title, const char* text, MBox type);
    bool Message(const std::string& title, const std::string& text, MBox type);
    bool Message(const wchar_t* title, const wchar_t* text, MBox type);
    bool Message(const std::wstring& title, const std::wstring& text, MBox type);

};