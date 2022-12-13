#pragma once
#include "../egxcommon.hpp"
#include <string_view>
#include <shaderc/shaderc.hpp>
#include <mutex>
#include <optional>
#include <json/json.hpp>

namespace egx
{

	namespace internal {
		struct ShaderCacheInformation;
	}

	struct ShaderReflection
	{
		struct {
			bool HasValue = false;
			std::string Name;
			uint32_t Offset;
			uint32_t Size;
		} Pushconstant;

		struct BindingInfo {
			uint32_t BindingId;
			uint32_t Size;
			uint32_t DescriptorCount;
			VkDescriptorType Type;
			uint32_t Dimension;
			bool IsBuffer;
			bool IsDynamic;
			std::string Name;
		};

		struct IO {
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

		VkShaderStageFlags ShaderStage;

		COMPGFX std::string DumpAsText() const;
	};

	enum class BindingAttributes : uint32_t {
		Default        = 0b001,
		DynamicUniform = 0b010,
		DynamicStorage = 0b100
	};

	class Shader2 {
	public:
		COMPGFX static ref<Shader2> FactoryCreate(
			const ref<VulkanCoreInterface>& CoreInterface,
			std::string_view File,
			BindingAttributes Attributes = BindingAttributes::Default
		);

		COMPGFX static ref<Shader2> FactoryCreateEx(
			const ref<VulkanCoreInterface>& CoreInterface,
			std::string_view Code,
			VkShaderStageFlags ShaderType,
			BindingAttributes Attributes,
			std::string_view OptionalFilePathForDebugging = ""
		);

		COMPGFX static void SetGlobalCacheFile(std::string_view CacheFolder);

		Shader2(Shader2&) = delete;
		Shader2& operator=(Shader2&) = delete;
		COMPGFX Shader2(Shader2&&) noexcept;
		COMPGFX Shader2& operator=(Shader2&&) noexcept;
		COMPGFX ~Shader2();

		template<typename T>
		void SetSpecializationConstant(int id, T data) {
			VkSpecializationMapEntry entry{};
			entry.constantID = id;
			entry.offset = _SpecializationOffset;
			entry.size = sizeof(T);
			uint8_t* data8 = (uint8_t*)&data;
			_SpecializationOffset += sizeof(T);
			for (int i = 0; i < sizeof(T); i++) {
				_SpecializationData.push_back(data8[i]);
			}
			_SpecializationConstants.push_back(entry);
		}

		VkSpecializationInfo GetSpecializationInfo() const {
			VkSpecializationInfo info{};
			info.mapEntryCount = (uint32_t)_SpecializationConstants.size();
			info.pMapEntries = _SpecializationConstants.data();
			info.dataSize = _SpecializationData.size();
			info.pData = _SpecializationData.data();
			return info;
		}

	public:
		VkShaderStageFlags ShaderStage;
		uint64_t CompilationDurationMilliseconds;
		std::vector<uint32_t> Bytecode;
		std::string SourceCode;
		VkShaderModule ShaderModule;
		BindingAttributes Attributes;
		ShaderReflection Reflection;
	private:
		shaderc_shader_kind _ShaderKind;
		ref<VulkanCoreInterface> _CoreInterface;
		uint32_t _SpecializationOffset = 0;
		std::vector<VkSpecializationMapEntry> _SpecializationConstants;
		std::vector<uint8_t> _SpecializationData;

	private:
		COMPGFX Shader2(
			VkShaderStageFlags shaderStage,
			uint64_t compileDuration,
			const std::vector<uint32_t>& bytecode,
			const std::string& sourceCode,
			VkShaderModule shaderModule,
			BindingAttributes attributes,
			ShaderReflection reflection,
			shaderc_shader_kind shaderKind,
			const ref<VulkanCoreInterface>& coreInterface) :
			ShaderStage(shaderStage),
			CompilationDurationMilliseconds(compileDuration),
			Bytecode(bytecode),
			SourceCode(sourceCode),
			ShaderModule(shaderModule),
			Attributes(attributes),
			Reflection(reflection),
			_ShaderKind(shaderKind),
			_CoreInterface(coreInterface)
		{}

		static std::vector<internal::ShaderCacheInformation> LoadCacheInformation();
		static void SaveCacheInformation(const std::vector<internal::ShaderCacheInformation>& cacheInformation);

		static std::optional<std::string> CacheFile;
		static std::mutex CacheWriteLock;

		static std::string PreprocessInclude(std::string_view CurrentFilePath, std::string_view SourceDirectory, std::string_view code);
		static ShaderReflection GenerateReflection(const std::vector<uint32_t>& Bytecode, BindingAttributes Attributes);

		static ref<Shader2> CreateFromCache(const ref<VulkanCoreInterface>& CoreInterface, const internal::ShaderCacheInformation& CacheInfo, BindingAttributes Attributes);

	};

}