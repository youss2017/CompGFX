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

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

namespace Utils
{

    bool Exists(const char *file_path);
    bool Exists(const std::string &file_path);
    bool Exists(const std::filesystem::path &file_path);
    void WriteBinaryFile(const char *file_path, void *buffer, uint64_t bufferSize);

    bool WriteTextFile(const char *file_path, std::string text);
    bool WriteTextFile(const std::string &file_path, std::string text);
    bool WriteTextFile(const std::filesystem::path &file_path, std::string text);

    template <typename T>
    static inline T ClampValues(T value, T min, T max)
    {
        if (value > max)
            value = max;
        else if (value < min)
            value = min;
        return value;
    }

    std::string ReadLine(FILE *file);
    std::vector<std::string> StringSplitter(std::string in_pattern, std::string &content);
    std::string StringCombiner(std::vector<std::string> &in_split);

    void CPUSleep(unsigned long long durationMillisecond);
    // asm {int 0} or DebugBreak() or raise(...)
    void Break();
    void FatalBreak(int ExitCode = 1);

    std::string WideStringToString(std::wstring &input_str);
    std::wstring StringToWideString(std::string &input_str);
    std::string LowerCaseString(std::string &input_str);
    std::wstring LowerCaseString(std::wstring &input_str);
    std::string LoadTextFile(const char *file_path);
    std::string LoadTextFile(const std::string &file_path);
    std::string LoadTextFile(const std::filesystem::path &file_path);

    bool CreateEmptyFile(const char *file_path);
    bool CreateEmptyFile(const std::string &file_path);
    bool CreateEmptyFile(const std::filesystem::path &file_path);

    std::vector<uint8_t> LoadBinaryFile(const char *file_path);
    std::vector<uint8_t> LoadBinaryFile(std::string &file_path);
    std::vector<uint8_t> LoadBinaryFile(const std::filesystem::path &file_path);

#include "common.inl"

}
