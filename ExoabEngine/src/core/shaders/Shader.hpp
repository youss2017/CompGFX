#pragma once
#include "../EngineBase.h"
#include <cstdint>
#include <vector>
#include <filesystem>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

#ifndef NULL
#define NULL 0
#endif

class Shader
{
public:
    Shader(GraphicsContext context, const char *ShaderPath, const char *EntryPointFunction = "main");
    ~Shader();
    const std::string &GetSource();
    inline uint32_t *GetBytecode() { return m_Bytecode.data(); }
    inline uint32_t GetWordCount() { return m_Bytecode.size(); }
    inline uint32_t GetByteSize() { return m_Bytecode.size() * 4; }
    inline bool GetCompileStatus() { return m_CompileStatus; }
    inline std::string GetShaderFilename() { return m_ShaderFilename; }
    inline std::string GetShaderDirectory() { return m_ShaderDirectory; }
    inline shaderc_shader_kind GetShaderKind() { return m_ShaderKind; }
    inline APIHandle GetShader() { return m_ShaderHandle; }
    inline const std::string& GetEntryPoint() { return m_EntryPointFunction; }

    // Call delete[] on byecode pointer when your done with it.
    static void CompileVulkanSPIRVText(const char *source_code, const char *filename, shaderc_shader_kind shader_type, uint32_t **pOutCode, uint32_t *pOutSize, const char *EntryPointFunction = "main");

private:
    GraphicsContext m_Context;
    std::vector<uint32_t> m_Bytecode;
    std::string m_ShaderDirectory, m_ShaderFilename, m_EntryPointFunction, m_ShaderPath;
    std::string m_Source;
    bool m_CompileStatus = false;
    shaderc_shader_kind m_ShaderKind;
    APIHandle m_ShaderHandle = NULL;

    static std::string s_CacheDirectory;

private:
    // Essentially copies data from #include "[file]" into the text since shaderc cannot do that.
    std::string ProcessIncludeDirectives(std::string source_code, std::string root_directory);
    void InitalizeVulkan();
    void InitalizeOpenGL();
    void EvaluateCaching(std::filesystem::path shader_path, bool& UsingCache, bool& Cached, std::string& identifier);

public:
    int m_ApiType;
    static bool ConfigureShaderCache(std::string SpirvCahceFolder);
};
