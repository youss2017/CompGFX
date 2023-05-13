#pragma once
#include <core/egx.hpp>
#include <shaderc/shaderc.hpp>
#include <vector>
#include <string_view>
#include <optional>
#include <map>

namespace egx
{

    enum class BindingAttributes : uint32_t
    {
        Default = 0b001,
        DynamicUniform = 0b010,
        DynamicStorage = 0b100
    };

    struct ShaderReflection
    {
        struct
        {
            bool HasValue = false;
            std::string Name;
            uint32_t Offset = 0;
            uint32_t Size = 0;
        } Pushconstant;

        struct BindingInfo
        {
            uint32_t BindingId;
            uint32_t Size;
            uint32_t DescriptorCount;
            VkDescriptorType Type;
            uint32_t Dimension;
            bool IsBuffer;
            bool IsDynamic;
            std::string Name;
        };

        struct IO
        {
            uint32_t Location;
            uint32_t Binding;
            uint32_t VectorSize;
            uint32_t Size;
            VkFormat Format;
            std::string Name;
        };

        // <BindingId, <Location, IO>
        std::map<uint32_t, std::map<uint32_t, IO>> IOBindingToManyLocationIn;
        // <BindingId, <Location, IO>
        std::map<uint32_t, std::map<uint32_t, IO>> IOBindingToManyLocationOut;
        // <SetId, <BindingId, BindingInfo>
        std::map<uint32_t, std::map<uint32_t, BindingInfo>> SetToManyBindings;
        std::map<std::string, BindingInfo> ResourceNameToBinding;
        VkShaderStageFlags ShaderStage = 0;

        std::string DumpAsText() const;
        // [Warning] Does not combine push constants.
        static ShaderReflection Combine(const std::initializer_list<ShaderReflection>& reflections);
    };

    struct ShaderDefines
    {
        std::vector<std::pair<std::string, std::string>> Defines;

        void Add(const std::string &preprocessor) { Defines.push_back({preprocessor, ""}); }
        void Add(const std::string &preprocessor, const std::string &value) { Defines.push_back({preprocessor, value}); }

        bool HasValue() { return Defines.size() > 0; }
    };

    class Shader
    {
    public:
        enum class Type
        {
            Vertex = VK_SHADER_STAGE_VERTEX_BIT,
            Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
            Compute = VK_SHADER_STAGE_COMPUTE_BIT,
            None = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM
        };

        struct PreprocessDefines
        {
            std::vector<std::pair<std::string, std::string>> Defines;

            void Add(const std::string &preprocessor) { Defines.push_back({preprocessor, ""}); }
            void Add(const std::string &preprocessor, const std::string &value) { Defines.push_back({preprocessor, value}); }

            bool HasValue() { return Defines.size() > 0; }
        };

    public:
        Shader() = default;
        Shader(const DeviceCtx &pCtx, const std::string &file,
               BindingAttributes attributes = BindingAttributes::Default,
               PreprocessDefines defines = {}, bool compileDebug = false,
               Type overrideType = Type::None);

        Shader(const DeviceCtx &pCtx, const std::string &sourceCode,
               Shader::Type type, BindingAttributes attributes = BindingAttributes::Default,
               PreprocessDefines defines = {}, bool compileDebug = false);

        void GetSourceCode(std::string &out) const;
        vk::ShaderModule GetModule() const { return m_Data->m_Module; }

        template <typename T>
        void SetSpecializationConstants(int id, T data)
        {
            VkSpecializationMapEntry entry{};
            entry.constantID = id;
            entry.offset = m_SpecializationOffset;
            entry.size = sizeof(T);
            uint8_t *data8 = (uint8_t *)&data;
            m_SpecializationOffset += sizeof(T);
            for (int i = 0; i < sizeof(T); i++)
            {
                m_SpecializationData.push_back(data8[i]);
            }
            m_SpecializationConstants.push_back(entry);
        }

        const vk::SpecializationInfo &GetSpecializationConstants() const
        {
            m_ConstantsInfo.mapEntryCount = (uint32_t)m_SpecializationConstants.size();
            m_ConstantsInfo.pMapEntries = m_SpecializationConstants.data();
            m_ConstantsInfo.dataSize = m_SpecializationData.size();
            m_ConstantsInfo.pData = m_SpecializationData.data();
            return m_ConstantsInfo;
        }

        void ClearSpecializationConstants()
        {
            m_SpecializationOffset = 0;
            m_SpecializationConstants.clear();
            m_SpecializationData.clear();
        }

        ShaderReflection Reflection() const
        {
            return m_Reflection;
        }

        Type GetType() const { return m_Type; }

        double CompilationDuration() const { return m_Duration; }

        static std::vector<vk::PushConstantRange> GetPushconstants(const std::initializer_list<Shader> &shaders);
        static std::map<uint32_t, vk::DescriptorSetLayout> CreateDescriptorSetLayouts(const std::initializer_list<Shader> &shaders);
        static std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayoutsAsScalar(const std::map<uint32_t, vk::DescriptorSetLayout>& setLayouts)
        {
            std::vector<vk::DescriptorSetLayout> scalar;
            for (auto &[_, setLayout] : setLayouts)
                scalar.push_back(setLayout);
            return scalar;
        }

    private:
        static std::string PreprocessIncludeFiles(std::string_view CurrentFilePath,
                                                  std::string_view SourceDirectory, std::string_view code,
                                                  std::vector<std::pair<std::string, uint64_t>> &includesLastModifiedDate,
                                                  std::optional<std::vector<std::string>> &pragmaOnce);

        static ShaderReflection GenerateReflection(const std::vector<uint32_t> &Bytecode, BindingAttributes Attributes);

        static std::vector<uint32_t> CompileGlslToBytecode(const std::string &sourceCode,
                                                           Type type, const PreprocessDefines &defines,
                                                           bool compileDebug, const std::string &fileName);

        struct DataWrapper
        {
            DeviceCtx m_Ctx;
            vk::ShaderModule m_Module = nullptr;

            DataWrapper() = default;
            DataWrapper(DataWrapper &) = delete;
            DataWrapper(DataWrapper &&) = delete;
            ~DataWrapper();
        };

    private:
        mutable std::string m_SourceCode;
        mutable vk::SpecializationInfo m_ConstantsInfo;
        std::shared_ptr<DataWrapper> m_Data;
        ShaderReflection m_Reflection;
        double m_Duration = 0.0;
        Type m_Type;

        uint32_t m_SpecializationOffset = 0;
        std::vector<vk::SpecializationMapEntry> m_SpecializationConstants;
        std::vector<uint8_t> m_SpecializationData;
    };

    class IShaderCache
    {
    public:
        IShaderCache(const DeviceCtx &pCtx) : m_Ctx(pCtx) {}

        virtual Shader LoadOrCompileFile(const std::string &name, const std::string &fileName, const Shader::PreprocessDefines &defines = {}, Shader::Type overrideType = Shader::Type::None) = 0;
        virtual Shader LoadOrCompileGlsl(const std::string &name, const std::string &sourceCode, Shader::Type type, const Shader::PreprocessDefines &defines = {}) = 0;

        virtual void ClearCache() = 0;

    protected:
        const DeviceCtx &m_Ctx;
    };

    class JsonShaderCache : public IShaderCache
    {
    public:
        // Registery that contains a list of all cached shaders and points to the file that stores the cache
        // Example:
        // registry.json --> [ {name: "....", file: "vert.json" } ]
        // vert.json
    };

}