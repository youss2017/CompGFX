#pragma once
#include "../egxcommon.hpp"
#include <cstdint>
#include <vector>
#include <filesystem>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include "../vulkinc.hpp"
#include <set>

namespace egx {

    struct ReflectionBinding {
        std::string Name;
        uint32_t Binding;
        uint32_t Size;
        uint32_t DescriptorCount;
        // If false then storage
        bool Uniform;
        bool Dynamic;
        VkShaderStageFlags ShaderStage;
    };

    class InputAssemblyDescription {

    public:
        InputAssemblyDescription() = default;
        InputAssemblyDescription(InputAssemblyDescription& copy) = default;
        InputAssemblyDescription(InputAssemblyDescription&& move) = default;
        InputAssemblyDescription& operator=(InputAssemblyDescription&& move) = default;

        inline void AddAttribute(
            uint32_t BindingId, uint32_t Location,
            uint32_t Size, VkFormat Format) {
            uint32_t offset = 0;
            for (auto& a : Attributes)
                if ((a.BindingId == BindingId))
                    offset += a.Size;
            Attributes.push_back(
                { BindingId, Location, offset, Size, Format }
            );
        }

        inline uint32_t GetStride(uint32_t BindingId) const {
            uint32_t stride = 0;
            for (auto& e : Attributes) {
                if (e.BindingId != BindingId) continue;
                stride += e.Size;
            }
            return stride;
        }

        inline std::set<uint32_t> GetBindings() const {
            std::set<uint32_t> bindings;
            for (auto& a : Attributes) {
                bindings.insert(a.BindingId);
            }
            return bindings;
        }

    public:
        struct attribute {
            uint32_t BindingId;
            uint32_t Location;
            uint32_t Offset;
            uint32_t Size;
            VkFormat Format;
        };
        std::vector<attribute> Attributes;
    };

    struct Pushconstant {
        VkShaderStageFlags ShaderStage;
        uint32_t Offset;
        uint32_t Size;
    };

    class Shader
    {
    public:
        EGX_API Shader(ref<VulkanCoreInterface> CoreInterface, const char* ShaderPath, const char* EntryPointFunction = "main");
        EGX_API Shader(Shader& cp) = delete;
        EGX_API Shader(Shader&& move) noexcept;
        EGX_API Shader& operator=(Shader&& move) noexcept;
        EGX_API ~Shader();
        EGX_API const std::string& GetSource();
        inline const uint32_t* GetBytecode() const { return m_Bytecode.data(); }
        inline uint32_t GetBytecodeSize() const { return (uint32_t)m_Bytecode.size() * 4u; }
        inline bool GetCompileStatus() const { return m_CompileStatus; }
        inline std::string GetShaderFilename() const { return m_ShaderFilename; }
        inline std::string GetShaderDirectory() const { return m_ShaderDirectory; }
        inline shaderc_shader_kind GetShaderKind() const { return m_ShaderKind; }
        inline VkShaderModule GetShader() const { return m_ShaderHandle; }
        inline const std::string& GetEntryPoint() const { return m_EntryPointFunction; }

        EGX_API std::string ReflectionInfo();

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

        EGX_API static void CompileVulkanSPIRVText(const char* source_code, const char* filename, shaderc_shader_kind shader_type, std::vector<uint32_t>& OutCode, const char* EntryPointFunction = "main");
        EGX_API static Shader CreateFromSource(const ref<VulkanCoreInterface>& CoreInterface, const char* source_code, shaderc_shader_kind shader_type);

    public:
        Pushconstant Pushconstants;
        std::map<uint32_t, std::map<uint32_t, ReflectionBinding>> Reflection;
        VkShaderStageFlags ShaderStage;

    private:
        friend class VulkanSwapchain;
        Shader() = default;

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