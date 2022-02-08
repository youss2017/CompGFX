#include "common.hpp"
#include <regex>
#include <algorithm>
#include <iterator>
#include <filesystem>

#ifdef _WIN32
#include <io.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <signal.h>

using namespace std;

namespace Utils
{

    bool Exists(const char *file_path)
    {
        /*
        #ifndef _WIN32
                if (access(filePath, F_OK) == 0)
                    return true;
                return false;
        #else
                if (_access(filePath, 06) == 0)
                    return true;
                else
                    return false;
        #endif
        */
        return std::filesystem::exists(file_path);
    }

    bool Exists(const std::string &file_path)
    {
        return std::filesystem::exists(file_path);
    }

    bool Exists(const std::filesystem::path &file_path)
    {
        return std::filesystem::exists(file_path);
    }

    void WriteBinaryFile(const char *filePath, void *buffer, uint64_t bufferSize)
    {
        FILE *handle = fopen(filePath, "wb");
        if (!handle)
        {
            string warn_message = "Could not write buffer to file, <";
            warn_message += filePath;
            warn_message += "]";
            logwarnings(warn_message);
            return;
        }
        fwrite(buffer, bufferSize, 1, handle);
        fflush(handle);
        fclose(handle);
    }

    bool WriteTextFile(const char *file_path, std::string text)
    {
        FILE *handle = fopen(file_path, "w");
        if (!handle)
            return false;
        fwrite(text.data(), text.size(), 1, handle);
        fclose(handle);
        return true;
    }

    bool WriteTextFile(const std::string &file_path, std::string text)
    {
        return WriteTextFile(file_path.c_str(), text);
    }

    bool WriteTextFile(const std::filesystem::path &file_path, std::string text)
    {
        std::string path = file_path.string();
        return WriteTextFile(path.c_str(), text);
    }

    std::string ReadLine(FILE *file)
    {
        int character = '\n';
        std::string line = "";
        do
        {
            character = fgetc(file);
            if (character == '\n')
                break;
            line += character;
        } while (!feof(file));
        return line;
    }

    // https://stackoverflow.com/questions/13172158/c-split-string-by-line
    std::vector<std::string> StringSplitter(std::string in_pattern, std::string &content)
    {
        using namespace std;
        std::vector<std::string> split_content;

        regex pattern(in_pattern);
        copy(sregex_token_iterator(content.begin(), content.end(), pattern, -1),
             sregex_token_iterator(), back_inserter(split_content));
        return split_content;
    }

    std::string StringCombiner(std::vector<std::string> &in_split)
    {
        std::string result;
        for (auto &split : in_split)
            result += split;
        return result;
    }

    void CPUSleep(unsigned long long durationMillisecond)
    {
#ifdef _WIN32
        Sleep(durationMillisecond);
#else
        usleep(durationMillisecond * 1000);
#endif
    }

    void Break()
    {
#if defined(_WIN32)
        DebugBreak();
#else
        raise(SIGTRAP);
#endif
    }

    void FatalBreak(int ExitCode)
    {
        log_error("!!!!!!!!!!!!!Fatal Debug Break!!!!!!!!!!!!!");
        Break();
        exit(ExitCode);
    }

    // TODO: Use stack allocation for smaller strings for the following functions
    std::string WideStringToString(std::wstring &input_str)
    {
        char *buffer = new char[input_str.length() * sizeof(wchar_t) + 1];
        wcstombs(buffer, &input_str[0], input_str.length() * sizeof(wchar_t));
        buffer[input_str.length() * sizeof(wchar_t)] = '\0';
        std::string result = buffer;
        delete[] buffer;
        return result;
    }

    std::wstring StringToWideString(std::string &input_str)
    {
        wchar_t *buffer = new wchar_t[input_str.length() + 1];
        mbstowcs(buffer, &input_str[0], input_str.length());
        buffer[input_str.length()] = L'\0';
        std::wstring result = buffer;
        delete[] buffer;
        return result;
    }

    std::string LowerCaseString(std::string &input_str)
    {
        char *buffer = new char[input_str.length() + 1];
        for (size_t i = 0; i < input_str.length(); i++)
            buffer[i] = tolower(input_str[i]);
        buffer[input_str.length()] = '\0';
        std::string result = buffer;
        delete[] buffer;
        return result;
    }

    std::wstring LowerCaseString(std::wstring &input_str)
    {
        wchar_t *buffer = new wchar_t[input_str.length() + 1];
        for (size_t i = 0; i < input_str.length(); i++)
            buffer[i] = towlower(input_str[i]);
        buffer[input_str.length()] = L'\0';
        std::wstring result = buffer;
        delete[] buffer;
        return result;
    }

    std::string LoadTextFile(const char *file_path)
    {
        std::string str;
        if (!std::filesystem::exists(file_path))
        {
            std::string err = "Could not load text file because it does not exist! <" + std::string(file_path) + ">";
            logerrors(err);
            str = "NULL " + std::string(file_path) + ", DOES NOT EXIST!";
            return str;
        }
        FILE *handle = fopen(file_path, "r");
        fseek(handle, 0, SEEK_END);
        size_t max_file_size = ftell(handle) + 1;
        char *buffer = new char[max_file_size];
        rewind(handle);
        size_t read_bytes = fread(buffer, 1, max_file_size, handle);
        fclose(handle);
        str = std::string(buffer, 0, read_bytes);
        delete[] buffer;
        return str.substr(0, read_bytes);
    }

    std::string LoadTextFile(const std::string &file_path)
    {
        return LoadTextFile(&file_path[0]);
    }

    std::string LoadTextFile(const std::filesystem::path &file_path)
    {
        std::string path = file_path.string();
        return LoadTextFile(path.c_str());
    }

    bool CreateEmptyFile(const char *file_path)
    {
        FILE *handle = fopen(file_path, "w");
        if (!handle)
            return false;
        fclose(handle);
        return true;
    }

    bool CreateEmptyFile(const std::string &file_path)
    {
        return CreateEmptyFile(file_path.c_str());
    }

    bool CreateEmptyFile(const std::filesystem::path &file_path)
    {
        std::string path = file_path.string();
        return CreateEmptyFile(path.c_str());
    }

    std::vector<uint8_t> LoadBinaryFile(const char *file_path)
    {
        FILE *handle = fopen(file_path, "rb");
        fseek(handle, 0, SEEK_END);
        long bsize = ftell(handle); // this is this the max size, and is probably bigger than the file itself
        char *buffer = new char[bsize];
        rewind(handle);
        long long read_bytes = fread(buffer, 1, bsize, handle);
        fclose(handle);
        std::vector<uint8_t> vbuf;
        vbuf.resize(read_bytes);
        memcpy(vbuf.data(), buffer, read_bytes);
        delete[] buffer;
        return vbuf;
    }

    std::vector<uint8_t> LoadBinaryFile(std::string &file_path)
    {
        return LoadBinaryFile(file_path.c_str());
    }

    std::vector<uint8_t> LoadBinaryFile(const std::filesystem::path &file_path)
    {
        std::string path = file_path.string();
        return LoadBinaryFile(path.c_str());
    }

};
