#pragma once
#include <string>
#include <cstring>
#include <climits>

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

namespace Utils
{
    bool GRAPHICS_API StrContains(const std::string &line, const char *str);
    bool GRAPHICS_API StrStartsWith(const std::string &line, const char *str);
    // returns -1 if could not find str
    int GRAPHICS_API StrFindFirstIndexOf(const std::string &line, const char *str);
    std::string GRAPHICS_API StrRemoveAll(const std::string &LINE, const char *str);
    std::string GRAPHICS_API StrTrim(const std::string &line);
    std::string GRAPHICS_API StrLowerCase(const std::string &line);
    std::string GRAPHICS_API StrUpperCase(const std::string &line);
    std::string GRAPHICS_API StrReplaceAll(const std::string& line, const char* Target, const char* Replacement);
    bool GRAPHICS_API EqualNotCaseSensitive(const std::string& a, const std::string& b);
    bool GRAPHICS_API EqualNotCaseSensitive(const std::string& a, const char* b);

    // fail is set to true if no number could be found
    // doesn't work with negative numbers (returns the number without the sign)
    int GRAPHICS_API StrGetFirstNumber(const std::string &line, bool &fail);
}
