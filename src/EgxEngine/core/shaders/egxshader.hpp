#pragma once
#include "../egxcommon.hpp"
#include <cstdint>
#include <vector>
#include <filesystem>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include "../vulkinc.hpp"

#ifndef NULL
#define NULL 0
#endif

namespace egx {

    class egxshader
    {
    public:
        EGX_API egxshader(ref<VulkanCoreInterface> CoreInterface, const char* ShaderPath, const char* EntryPointFunction = "main");
        EGX_API egxshader(egxshader& cp) = delete;
        EGX_API egxshader(egxshader&& move);
        EGX_API egxshader& operator=(egxshader& move);
        EGX_API ~egxshader();
        EGX_API const std::string& GetSource();
        inline const uint32_t* GetBytecode() const { return m_Bytecode.data(); }
        inline uint32_t GetBytecodeSize() const { return (uint32_t)m_Bytecode.size() * 4u; }
        inline bool GetCompileStatus() const { return m_CompileStatus; }
        inline std::string GetShaderFilename() const { return m_ShaderFilename; }
        inline std::string GetShaderDirectory() const { return m_ShaderDirectory; }
        inline shaderc_shader_kind GetShaderKind() const { return m_ShaderKind; }
        inline VkShaderModule GetShader() const { return m_ShaderHandle; }
        inline const std::string& GetEntryPoint() const { return m_EntryPointFunction; }

        template<typename T>
        void SetSpecializationConstant(int id, T data) {
            VkSpecializationMapEntry entry{};
            entry.constantID = id;
            entry.offset = mSpecializationOffset;
            entry.size = sizeof(T);
            char* data8 = (char*)&data;
            mSpecializationOffset += sizeof(T);
            for (int i = 0; i < sizeof(T); i++) {
                mSpecializationData.push_back(data8[i]);
            }
            mSpecializationConstants.push_back(entry);
        }

        VkSpecializationInfo GetSpecializationInfo() const {
            VkSpecializationInfo info{};
            info.mapEntryCount = (uint32_t)mSpecializationConstants.size();
            info.pMapEntries = mSpecializationConstants.data();
            info.dataSize = mSpecializationData.size();
            info.pData = mSpecializationData.data();
            return info;
        }

        // Call delete[] on byecode pointer when your done with it.
        EGX_API static void CompileVulkanSPIRVText(const char* source_code, const char* filename, shaderc_shader_kind shader_type, uint32_t** pOutCode, uint32_t* pOutSize, const char* EntryPointFunction = "main");

    private:
        ref<VulkanCoreInterface> CoreInterface;
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
        static EGX_API bool ConfigureShaderCache(std::string SpirvCahceFolder);
    };

}