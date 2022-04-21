#include "StringUtils.hpp"
#include "Profiling.hpp"
#include <vector>
#include <algorithm>

namespace Utils
{

    bool GRAPHICS_API StrContains(const std::string &line, const char *str)
    {
        int strPos = 0;
        std::string temp = "";
        for (size_t i = 0; i < line.size(); i++)
        {
            if (line[i] == str[strPos])
            {
                strPos++;
                temp += line[i];
                if (strcmp(temp.c_str(), str) == 0)
                {
                    return true;
                }
            }
            else
            {
                strPos = 0;
                temp = "";
            }
        }
        return false;
    }

    bool GRAPHICS_API StrStartsWith(const std::string &line, const char *str)
    {
        size_t length = strlen(str);
        for (size_t i = 0; i < length; i++)
        {
            if (line[i] != str[i])
                return false;
        }
        return true;
    }

    // returns -1 if could not find str
    int GRAPHICS_API StrFindFirstIndexOf(const std::string &line, const char *str)
    {
        int strPos = 0;
        int linePos = 0;
        std::string temp = "";
        for (size_t i = 0; i < line.size(); i++)
        {
            if (line[i] == str[strPos])
            {
                if (strPos == 0)
                    linePos = i;
                strPos++;
                temp += line[i];
                if (strcmp(temp.c_str(), str) == 0)
                {
                    return linePos;
                }
            }
            else
            {
                strPos = 0;
                temp = "";
            }
        }
        return -1;
    }

    std::string GRAPHICS_API StrRemoveAll(const std::string &LINE, const char *str)
    {
        size_t length = strlen(str);
        int pos = -1;
        std::vector<int> strPositions;
        std::string temp = LINE;
        while ((pos = StrFindFirstIndexOf(temp, str)) != -1)
        {
            strPositions.push_back(pos);
            temp = temp.substr(pos + length);
        }
        if (strPositions.size() == 0)
            return LINE;

        std::string result = "";
        int lastPosition = 0;
        for (auto position : strPositions)
        {
            result += LINE.substr(lastPosition, position);
            lastPosition += position + length;
        }
        return result;
    }

    std::string GRAPHICS_API StrTrim(const std::string &line)
    {
        size_t startPos = 0;
        for (startPos = 0; startPos < line.size(); startPos++)
        {
            if (line[startPos] != ' ')
            {
                break;
            }
        }
        int endPos = 0;
        for (endPos = line.size(); endPos > 0; endPos--)
        {
            if (line[endPos] != ' ')
            {
                break;
            }
        }
        return line.substr(startPos, endPos - startPos);
    }

    std::string GRAPHICS_API StrLowerCase(const std::string &line)
    {
        std::string result;
        result.reserve(line.size());
        for (size_t i = 0; i < line.size(); i++)
            result += tolower(line[i]);
        return result;
    }

    std::string GRAPHICS_API StrUpperCase(const std::string &line)
    {
        std::string result;
        result.reserve(line.size());
        for (size_t i = 0; i < line.size(); i++)
            result += toupper(line[i]);
        return result;
    }

    int GRAPHICS_API StrGetFirstNumber(const std::string &line, bool &fail)
    {
        auto isNumber = [](char c) throw()->int
        {
            char numbersList[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
            int i = 0;
            while (i < 10)
            {
                if (c == numbersList[i])
                    return i;
                i++;
            }
            return -1;
        };
        size_t stringIndex = 0;
        int result = 0;
        bool IsNegative = false;
        fail = true;
        while (stringIndex < line.size())
        {
            int v = isNumber(line[stringIndex]);
            if (v != -1)
            {
                result *= 10;
                result += v;
                fail = false;
            }
            else if (line[stringIndex] == '-')
            {
                if (IsNegative)
                    break;
                else
                    IsNegative = true;
            }
            else
            {
                if (fail == false)
                    break;
            }
            stringIndex++;
        }
        if (IsNegative)
            result *= -1;
        return result;
    }

    std::string GRAPHICS_API StrReplaceAll(const std::string &line, const char *Target, const char *Replacement)
    {
        std::string str = line;
        std::string from = Target;
        std::string to = Replacement;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }

    bool GRAPHICS_API EqualNotCaseSensitive(const std::string& a, const std::string& b)
    {
        std::string _a = Utils::StrLowerCase(a);
        std::string _b = Utils::StrLowerCase(b);
        return _a.compare(_b) == 0;
    }

    bool GRAPHICS_API EqualNotCaseSensitive(const std::string& a, const char* b)
    {
        std::string _a = Utils::StrLowerCase(a);
        size_t size = strlen(b);
        char *_cpy = (char*)alloca(size + 1);
        for(size_t i = 0; i < size; i++)
            _cpy[i] = tolower(b[i]);
        _cpy[size] = '\0';
        return _a.compare(_cpy) == 0;
    }

}