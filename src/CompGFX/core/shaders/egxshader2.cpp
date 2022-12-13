#include "egxshader2.hpp"
#include <Utility/CppUtility.hpp>
#include <stdexcept>
#include <filesystem>
#include <json/json.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <ranges>
#include "../memory/formatsize.hpp"
#include <filesystem>
using namespace nlohmann;
using namespace egx;
using namespace cpp;

std::optional<std::string> Shader2::CacheFile;
std::mutex Shader2::CacheWriteLock;

constexpr const char* CacheFileName = "GlobalShaderCache";
constexpr const char* CacheFileExtension = ".json";

namespace egx::internal {
	using namespace nlohmann;

	struct ShaderCacheInformation
	{
		std::string FilePath;
		std::string SourceCode;
		uint32_t ShaderStage{};
		std::vector<uint32_t> Bytecode;
		uint64_t LastModifiedDate{};
	};

	void to_json(json& j, const ShaderCacheInformation& p) {
		j = json{
			{"filePath", p.FilePath},
			{"sourceCode", p.SourceCode},
			{"shaderStage", p.ShaderStage},
			{"byteCode", p.Bytecode},
			{"lastModifiedDate", p.LastModifiedDate}
		};
	}

	void from_json(const json& j, ShaderCacheInformation& p) {
		j.at("filePath").get_to(p.FilePath);
		j.at("sourceCode").get_to(p.SourceCode);
		j.at("shaderStage").get_to(p.ShaderStage);
		j.at("byteCode").get_to(p.Bytecode);
		j.at("lastModifiedDate").get_to(p.LastModifiedDate);
	}
}

ref<Shader2> egx::Shader2::FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface, std::string_view File, BindingAttributes Attributes)
{
	if (!Exists(File)) {
		LOGEXCEPT("\"{0}\" does not exist.", File.data());
	}
	std::filesystem::path fileInfo(File);
	if (!fileInfo.has_extension()) {
		LOGEXCEPT("\"{0}\" has no file extention; cannot deduce shader type.", File.data());
	}

	if (Shader2::CacheFile.has_value()) {
		auto cacheList = Shader2::LoadCacheInformation();
		auto fullPath = fileInfo.string();
		auto cache = std::ranges::find_if(cacheList, [&](const internal::ShaderCacheInformation& info) { return EqualIgnoreCase(fullPath, info.FilePath); });
		if (cache != cacheList.end()) {
			if (std::filesystem::last_write_time(File).time_since_epoch().count() == cache->LastModifiedDate) {
				return CreateFromCache(CoreInterface, *cache, Attributes);
			}
		}
	}

	auto extension = fileInfo.extension().string();
	VkShaderStageFlags shaderStage{};
	if (EqualIgnoreCase(extension, ".vert")) {
		shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
	}
	else if (EqualIgnoreCase(extension, ".frag")) {
		shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	else if (EqualIgnoreCase(extension, ".comp")) {
		shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
	}
	else {
		LOGEXCEPT("\"{0}\" has unknown extension '{1}'. Supported extensions are .vert, .frag, and .comp", File.data(), extension);
	}

	auto code = ReadAllText(File);
	if (!code.has_value()) {
		LOGEXCEPT("Could not load file \"{0}\"", File.data());
	}
	auto processedCode = PreprocessInclude(fileInfo.string(), std::filesystem::absolute(fileInfo).parent_path().string(), *code);
	return FactoryCreateEx(CoreInterface, processedCode, shaderStage, Attributes, fileInfo.string());
}

ref<Shader2> egx::Shader2::FactoryCreateEx
(const ref<VulkanCoreInterface>& CoreInterface, std::string_view Code, VkShaderStageFlags ShaderType,
	BindingAttributes Attributes,
	std::string_view OptionalFilePathForDebugging)
{
	shaderc_shader_kind shaderKind{};
	switch (ShaderType) {
	case VK_SHADER_STAGE_VERTEX_BIT: shaderKind = shaderc_shader_kind::shaderc_vertex_shader; break;
	case VK_SHADER_STAGE_FRAGMENT_BIT: shaderKind = shaderc_shader_kind::shaderc_fragment_shader; break;
	case VK_SHADER_STAGE_COMPUTE_BIT: shaderKind = shaderc_shader_kind::shaderc_compute_shader; break;
	default:
		LOGEXCEPT("Invalid Shader Type, supported types are vertex, fragment, and compute.");
	}
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, VK_VERSION_1_2);
#ifdef _DEBUG
	options.SetOptimizationLevel(shaderc_optimization_level_zero);
	options.SetGenerateDebugInfo();
#else
	options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif
	auto start = std::chrono::high_resolution_clock::now();
	auto compilationResult = compiler.CompileGlslToSpv(Code.data(), shaderKind, "main.glsl", "main", options);
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	if (compilationResult.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success)
	{
		if (OptionalFilePathForDebugging.length() > 0) {
			LOGEXCEPT("Encountered error during compilation.\nFile:{0}\n{1}", OptionalFilePathForDebugging.data(), compilationResult.GetErrorMessage());
		}
		LOGEXCEPT("Encountered error during compilation.\n{1}", compilationResult.GetErrorMessage());
	}
	if (compilationResult.GetNumWarnings() > 0) {
		if (OptionalFilePathForDebugging.length() > 0) {
			LOG(WARNING, "{0} Warnings for Shader \"{1}\"", compilationResult.GetNumWarnings(), OptionalFilePathForDebugging.data());
		}
	}
	auto bytecode = std::vector<uint32_t>(compilationResult.begin(), compilationResult.end());
	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = bytecode.size() * sizeof(uint32_t);
	createInfo.pCode = bytecode.data();
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(CoreInterface->Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		LOGEXCEPT("Could not create shader module for '{0}'.", OptionalFilePathForDebugging.data());
	}
	auto reflection = GenerateReflection(bytecode, Attributes);
	return new Shader2(ShaderType, duration, bytecode, Code.data(), shaderModule, Attributes, reflection, shaderKind, CoreInterface);
}

std::string egx::Shader2::PreprocessInclude(std::string_view CurrentFilePath, std::string_view SourceDirectory, std::string_view code)
{
	std::string output;
	auto lines = Split(code.data(), "\n");
	int line_index = 1;
	for (auto& item : lines) {
		if (!StartsWith(item, "#include")) {
			output += item;
			output.push_back('\n');
			line_index++;
			continue;
		}
		auto include_line = Split(ReplaceAll(ReplaceAll(ReplaceAll(item, "\"", ""), "<", ""), ">", ""), "\\s+");
		if (include_line.size() < 2) {
			LOGEXCEPT("Invalid #include in \"{0}\" at line {1}", CurrentFilePath.data(), line_index);
		}
		auto& include_file = include_line[1];
		std::string include_file_path;
		try {
			include_file_path = (std::filesystem::path(SourceDirectory) / include_file).string();
			if (!std::filesystem::exists(include_file_path)) {
				LOGEXCEPT("#include is not found in \"{0}\" at line {1}", CurrentFilePath.data(), line_index);
			}
		}
		catch (...) {
			LOGEXCEPT("#include is malformed in \"{0}\" at line {1}", CurrentFilePath.data(), line_index);
		}
		auto include_source_code = ReadAllText(include_file_path);
		if (!include_source_code.has_value()) {
			LOGEXCEPT("Could not open #include file (READ FAIL) in \"{0}\" at line {1}", CurrentFilePath.data(), line_index);
		}
		std::filesystem::path p(include_file_path);
		include_source_code = PreprocessInclude(include_file_path, p.parent_path().string(), *include_source_code);
		output += *include_source_code;
		output.push_back('\n');
		output += "#line " + std::to_string(line_index);
		output.push_back('\n');
		line_index++;
	}
	return output;
}

void Shader2::SetGlobalCacheFile(std::string_view CacheFolder)
{
	std::lock_guard guard(Shader2::CacheWriteLock);
	if (std::filesystem::create_directories(CacheFolder)) {
		const char* mode = nullptr;
#ifdef _DEBUG
		mode = "DEBUG";
#else
		mode = "RELEASE";
#endif
		std::filesystem::path path = std::filesystem::path(CacheFolder) / Format("{0}_{1}{2}", CacheFileName, mode, CacheFileExtension);
		if (!std::filesystem::exists(path)) {
			CreateEmptyFile(path.string());
		}
		Shader2::CacheFile = path.string();
	}
	else {
		LOG(WARNING, "Could not create '{0}' folder tree for Shader cache.", CacheFolder.data());
	}
}

egx::Shader2::Shader2(Shader2&& move) noexcept
{
	memcpy(this, &move, sizeof(Shader2));
	memset(&move, 0, sizeof(Shader2));
}

COMPGFX Shader2& egx::Shader2::operator=(Shader2&& move) noexcept
{
	if (this == &move) return *this;
	if (ShaderModule) {
		vkDestroyShaderModule(_CoreInterface->Device, ShaderModule, nullptr);
	}
	memcpy(this, &move, sizeof(Shader2));
	memset(&move, 0, sizeof(Shader2));
	return *this;
}

egx::Shader2::~Shader2()
{
	if (ShaderModule) {
		vkDestroyShaderModule(_CoreInterface->Device, ShaderModule, nullptr);
	}
}

std::vector<internal::ShaderCacheInformation> Shader2::LoadCacheInformation()
{
	if (!Shader2::CacheFile.has_value() || Shader2::CacheFile->length() == 0) return {};
	std::lock_guard guard(Shader2::CacheWriteLock);
	auto cache = ReadAllText(*Shader2::CacheFile);
	if (!cache.has_value()) return {};
	return json(*cache).get<std::vector<internal::ShaderCacheInformation>>();
}

void Shader2::SaveCacheInformation(const std::vector<internal::ShaderCacheInformation>& cacheInformation)
{
	if (!Shader2::CacheFile.has_value() || Shader2::CacheFile->length() == 0) return;
	std::lock_guard guard(Shader2::CacheWriteLock);
	json out = cacheInformation;
	WriteAllText(*Shader2::CacheFile, out.dump(4));
}

ShaderReflection Shader2::GenerateReflection(const std::vector<uint32_t>& Bytecode, BindingAttributes Attributes)
{
	ShaderReflection output;
	spirv_cross::Compiler compiler(Bytecode);
	const auto& resources = compiler.get_shader_resources();

	auto readStageIO = [&](
		const spirv_cross::SmallVector<spirv_cross::Resource>& stageIO,
		std::map<uint32_t, std::map<uint32_t, ShaderReflection::IO>>& out) {
			for (auto& item : stageIO) {
				ShaderReflection::IO io{};
				const auto& type = compiler.get_type(item.type_id);
				auto baseType = type.basetype;
				io.Location = compiler.get_decoration(item.id, spv::DecorationLocation);
				io.Binding = compiler.get_decoration(item.id, spv::DecorationBinding);
				io.VectorSize = type.vecsize;
				VkFormat format{};
				if (baseType == type.Float) {
					io.Size = sizeof(float) * type.vecsize;
					switch (type.vecsize) {
					case 1: format = VK_FORMAT_R32_SFLOAT; break;
					case 2: format = VK_FORMAT_R32G32_SFLOAT; break;
					case 3: format = VK_FORMAT_R32G32B32_SFLOAT; break;
					case 4: format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
					}
				}
				else if (baseType == type.Int) {
					io.Size = sizeof(int32_t) * type.vecsize;
					switch (type.vecsize) {
					case 1: format = VK_FORMAT_R32_SINT; break;
					case 2: format = VK_FORMAT_R32G32_SINT; break;
					case 3: format = VK_FORMAT_R32G32B32_SINT; break;
					case 4: format = VK_FORMAT_R32G32B32A32_SINT; break;
					}
				}
				else if (baseType == type.UInt) {
					io.Size = sizeof(uint32_t) * type.vecsize;
					switch (type.vecsize) {
					case 1: format = VK_FORMAT_R32_UINT; break;
					case 2: format = VK_FORMAT_R32G32_UINT; break;
					case 3: format = VK_FORMAT_R32G32B32_UINT; break;
					case 4: format = VK_FORMAT_R32G32B32A32_UINT; break;
					}
				}
				else {
					LOGEXCEPT("Cannot Generate Reflection Supported Types are Float32, Int32, and UInt32");
				}
				io.Format = format;
				io.Name = compiler.get_name(item.id);
				out[io.Binding][io.Location] = io;
			}
	};
	readStageIO(resources.stage_inputs, output.IOBindingToManyLocationIn);
	readStageIO(resources.stage_outputs, output.IOBindingToManyLocationOut);

	if (resources.push_constant_buffers.size() > 0) {
		auto& buffer = resources.push_constant_buffers[0];
		auto& type = compiler.get_type(buffer.type_id);
		output.Pushconstant.Name = compiler.get_name(buffer.id);
		output.Pushconstant.Offset = (uint32_t)compiler.get_decoration(buffer.id, spv::DecorationOffset);
		output.Pushconstant.Size = (uint32_t)compiler.get_declared_struct_size(type);
		output.Pushconstant.HasValue = true;
	}

	auto readBufferResources = [&](
		const spirv_cross::SmallVector<spirv_cross::Resource>& buffers,
		VkDescriptorType typeNonDynamic, VkDescriptorType typeDynamic) {
			for (auto& item : buffers) {
				auto& type = compiler.get_type(item.type_id);
				ShaderReflection::BindingInfo info{};
				info.IsDynamic = ((uint32_t)Attributes & (uint32_t)BindingAttributes::DynamicUniform);
				info.Type = info.IsDynamic ? typeDynamic : typeNonDynamic;
				info.Name = compiler.get_name(item.id);
				info.BindingId = compiler.get_decoration(item.id, spv::DecorationBinding);
				info.Size = (uint32_t)compiler.get_declared_struct_size(type);
				info.DescriptorCount = std::max(type.array[0], 1u);
				info.IsBuffer = true;
				auto setId = compiler.get_decoration(item.id, spv::DecorationDescriptorSet);
				output.SetToManyBindings[setId][info.BindingId] = info;
			}
	};

	readBufferResources(resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
	readBufferResources(resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);

	auto readImageResources = [&](
		const spirv_cross::SmallVector<spirv_cross::Resource>& images, VkDescriptorType Type) {
			for (auto& item : images) {
				auto& type = compiler.get_type(item.type_id);
				ShaderReflection::BindingInfo info{};
				info.IsDynamic = false;
				info.Type = Type;
				info.Name = compiler.get_name(item.id);
				info.BindingId = compiler.get_decoration(item.id, spv::DecorationBinding);
				info.Size = 0;
				info.Dimension = type.image.dim;
				info.DescriptorCount = std::max(type.array[0], 1u);
				info.IsBuffer = false;
				auto setId = compiler.get_decoration(item.id, spv::DecorationDescriptorSet);
				output.SetToManyBindings[setId][info.BindingId] = info;
			}
	};
	readImageResources(resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	readImageResources(resources.sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	readImageResources(resources.separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
	// (TODO) Read Samplers?
	//readImageResources(resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER);

	return output;
}

ref<Shader2> Shader2::CreateFromCache(const ref<VulkanCoreInterface>& CoreInterface, const internal::ShaderCacheInformation& CacheInfo, BindingAttributes Attributes)
{
	auto reflection = GenerateReflection(CacheInfo.Bytecode, Attributes);
	shaderc_shader_kind shaderKind{};
	if (CacheInfo.ShaderStage & VK_SHADER_STAGE_VERTEX_BIT) {
		shaderKind = shaderc_shader_kind::shaderc_vertex_shader;
	}
	else if (CacheInfo.ShaderStage & VK_SHADER_STAGE_FRAGMENT_BIT) {
		shaderKind = shaderc_shader_kind::shaderc_fragment_shader;
	}
	else if (CacheInfo.ShaderStage & VK_SHADER_STAGE_COMPUTE_BIT) {
		shaderKind = shaderc_shader_kind::shaderc_compute_shader;
	}
	else {
		LOGEXCEPT("Shader Kind not supported (CreateFromCache())");
	}
	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = CacheInfo.Bytecode.size() * sizeof(uint32_t);
	createInfo.pCode = CacheInfo.Bytecode.data();
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(CoreInterface->Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		LOGEXCEPT("Could not create shader module.");
	}
	return new Shader2(CacheInfo.ShaderStage, 0ull, CacheInfo.Bytecode, CacheInfo.SourceCode, shaderModule, Attributes, reflection, shaderKind, CoreInterface);
}


std::string ShaderReflection::DumpAsText() const
{
	std::string output;

	for (auto& [binding, item_] : IOBindingToManyLocationIn) {
		for (auto& [location, item] : item_) {
			output +=
				Format("{{Binding={0} Location={1} Format={2} Size={3}}} {4} In;\n",
					item.Binding, item.Location,
					_internal::DebugVkFormatToString(item.Format), item.Size,
					item.Name);
		}
	}

	for (auto& [binding, item_] : IOBindingToManyLocationOut) {
		for (auto& [location, item] : item_) {
			output +=
				Format("{{Binding={0} Location={1} Format={2} Size={3}}} {4} Out;\n",
					item.Binding, item.Location,
					_internal::DebugVkFormatToString(item.Format), item.Size,
					item.Name);
		}
	}

	if (Pushconstant.HasValue) {
		output += Format("(push_constant) {{Offset={0} Size={1}}} {2}\n", Pushconstant.Offset, Pushconstant.Size, Pushconstant.Name);
	}

	for (auto& [setId, binding] : SetToManyBindings) {
		for (auto& [bindingId, item] : binding) {
			output += Format("(set={0}, binding={1}) {{Size={2} ArrayCount={3}}} {4}\n", setId, bindingId, item.Size, item.DescriptorCount, item.Name);
		}
	}

	return output;
}