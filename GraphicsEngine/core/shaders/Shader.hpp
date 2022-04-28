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
    GRAPHICS_API Shader(vk::VkContext context, const char *ShaderPath, const char *EntryPointFunction = "main");
    GRAPHICS_API ~Shader();
    GRAPHICS_API const std::string &GetSource();
    GRAPHICS_API inline uint32_t *GetBytecode() { return m_Bytecode.data(); }
    GRAPHICS_API inline uint32_t GetWordCount() { return m_Bytecode.size(); }
    GRAPHICS_API inline uint32_t GetByteSize() { return m_Bytecode.size() * 4; }
    GRAPHICS_API inline bool GetCompileStatus() { return m_CompileStatus; }
    GRAPHICS_API inline std::string GetShaderFilename() { return m_ShaderFilename; }
    GRAPHICS_API inline std::string GetShaderDirectory() { return m_ShaderDirectory; }
    GRAPHICS_API inline shaderc_shader_kind GetShaderKind() { return m_ShaderKind; }
    GRAPHICS_API inline VkShaderModule GetShader() { return m_ShaderHandle; }
    GRAPHICS_API inline const std::string& GetEntryPoint() { return m_EntryPointFunction; }

    template<typename T>
    void SetSpecializationConstant(int id, T data) {
        VkSpecializationMapEntry entry;
        entry.constantID = id;
        entry.offset = mSpecializationOffset;
        entry.size = sizeof(T);
        char* data8 = (char*)&data;
        mSpecializationOffset += sizeof(T);
        for(int i = 0; i < sizeof(T); i++) {
            mSpecializationData.push_back(data8[i]);
        }
        mSpecializationConstants.push_back(entry);
    }

    // Make sure to use the struct before Shader is destroyed.
    GRAPHICS_API VkSpecializationInfo GetSpecializationInfo() {
        VkSpecializationInfo info;
        info.mapEntryCount = mSpecializationConstants.size();
        info.pMapEntries = mSpecializationConstants.data();
        info.dataSize = mSpecializationData.size();
        info.pData = mSpecializationData.data();
        return info;
    }

    // Call delete[] on byecode pointer when your done with it.
    GRAPHICS_API static void CompileVulkanSPIRVText(const char *source_code, const char *filename, shaderc_shader_kind shader_type, uint32_t **pOutCode, uint32_t *pOutSize, const char *EntryPointFunction = "main");

private:
    vk::VkContext m_Context;
    std::vector<uint32_t> m_Bytecode;
    std::string m_ShaderDirectory, m_ShaderFilename, m_EntryPointFunction, m_ShaderPath;
    std::string m_Source;
    bool m_CompileStatus = false;
    shaderc_shader_kind m_ShaderKind;
    VkShaderModule m_ShaderHandle = NULL;
    unsigned int mSpecializationOffset = 0;
    std::vector<VkSpecializationMapEntry> mSpecializationConstants;
    std::vector<char> mSpecializationData;

    static std::string s_CacheDirectory;

private:
    // Essentially copies data from #include "[file]" into the text since shaderc cannot do that.
    std::string ProcessIncludeDirectives(std::string source_code, std::string root_directory);
    void InitalizeVulkan();
    void EvaluateCaching(std::filesystem::path shader_path, bool& UsingCache, bool& Cached, std::string& identifier);

public:
    GRAPHICS_API static bool ConfigureShaderCache(std::string SpirvCahceFolder);
};
