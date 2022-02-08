#pragma once
#include <string>
#include <cstring>
#include <climits>

namespace Utils
{
    bool StrContains(const std::string &line, const char *str);
    bool StrStartsWith(const std::string &line, const char *str);
    // returns -1 if could not find str
    int StrFindFirstIndexOf(const std::string &line, const char *str);
    std::string StrRemoveAll(const std::string &LINE, const char *str);
    std::string StrTrim(const std::string &line);
    std::string StrLowerCase(const std::string &line);
    std::string StrUpperCase(const std::string &line);
    std::string StrReplaceAll(const std::string& line, const char* Target, const char* Replacement);
    bool EqualNotCaseSensitive(const std::string& a, const std::string& b);
    bool EqualNotCaseSensitive(const std::string& a, const char* b);

    // fail is set to true if no number could be found
    // doesn't work with negative numbers (returns the number without the sign)
    int StrGetFirstNumber(const std::string &line, bool &fail);
}
