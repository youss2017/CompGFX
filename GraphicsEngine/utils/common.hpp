#pragma once
#include "Logger.hpp"
#include "Profiling.hpp"
#include "defines.h"
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

#ifndef GRAPHICS_API
#ifdef BUILD_GRAPHICS_DLL
#define GRAPHICS_API _declspec(dllexport)
#else
#define GRAPHICS_API _declspec(dllimport)
#endif
#endif

namespace Utils
{

    bool GRAPHICS_API Exists(const char *file_path);
    bool GRAPHICS_API Exists(const std::string &file_path);
    bool GRAPHICS_API Exists(const std::filesystem::path &file_path);
    void GRAPHICS_API WriteBinaryFile(const char *file_path, void *buffer, uint64_t bufferSize);

    bool GRAPHICS_API WriteTextFile(const char *file_path, std::string text);
    bool GRAPHICS_API WriteTextFile(const std::string &file_path, std::string text);
    bool GRAPHICS_API WriteTextFile(const std::filesystem::path &file_path, std::string text);

    template <typename T>
    static inline T ClampValues(T value, T min, T max)
    {
        if (value > max)
            value = max;
        else if (value < min)
            value = min;
        return value;
    }

    std::string GRAPHICS_API ReadLine(FILE *file);
    std::vector<std::string> GRAPHICS_API StringSplitter(std::string in_pattern, std::string &content);
    std::string GRAPHICS_API StringCombiner(std::vector<std::string> &in_split);

    void GRAPHICS_API CPUSleep(unsigned long long durationMillisecond);
    // asm {int 0} or DebugBreak() or raise(...)
    void GRAPHICS_API Break();
    void GRAPHICS_API FatalBreak(int ExitCode = 1);

    std::string GRAPHICS_API WideStringToString(std::wstring& input_str);
    std::wstring GRAPHICS_API StringToWideString(std::string& input_str);
    std::string GRAPHICS_API LowerCaseString(std::string& input_str);
    std::wstring GRAPHICS_API LowerCaseString(std::wstring& input_str);
    std::string GRAPHICS_API LoadTextFile(const char* file_path);
    std::string GRAPHICS_API LoadTextFile(const std::string& file_path);
    std::string GRAPHICS_API LoadTextFile(const std::filesystem::path &file_path);

    bool GRAPHICS_API CreateEmptyFile(const char* file_path);
    bool GRAPHICS_API CreateEmptyFile(const std::string& file_path);
    bool GRAPHICS_API CreateEmptyFile(const std::filesystem::path &file_path);

    std::vector<uint8_t> GRAPHICS_API LoadBinaryFile(const char *file_path);
    std::vector<uint8_t> GRAPHICS_API LoadBinaryFile(std::string &file_path);
    std::vector<uint8_t> GRAPHICS_API LoadBinaryFile(const std::filesystem::path &file_path);

#include "common.inl"

}
