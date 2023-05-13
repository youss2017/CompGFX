#include "egxshader2.hpp"
#include <Utility/CppUtility.hpp>
#include <stdexcept>
#include <filesystem>
#include <json.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <ranges>
#include "../memory/formatsize.hpp"
#include <filesystem>
#include <json.hpp>
#ifdef _WIN32
#include <winsqlite/winsqlite3.h>
#else
#error
#endif
using namespace nlohmann;
using namespace egx;
using namespace cpp;

std::optional<std::string> Shader2::CacheFile;
std::mutex Shader2::CacheWriteLock;
static sqlite3* GlobalDb = nullptr;

constexpr const char* CacheFileName = "GlobalShaderCache";
constexpr const char* CacheFileExtension = ".json";
constexpr bool LazyLoadShaderSourceCode = true;

namespace egx::internal {
	struct ShaderCacheInformation
	{
		uint32_t Id;
		uint32_t ShaderStage{};
		uint64_t LastModifiedDate{};
		std::string FilePath;
		std::vector<uint32_t> Bytecode;
		std::vector<std::pair<std::string, uint64_t>> IncludesLastModified;
	};
}

egx::Shader2::Shader2(DeviceCtx* pCtx, 
	const std::string& file,
	BindingAttributes attributes,
	Shader2Defines defines,
	bool compileDebug)
{
	if (!Exists(file)) {
		LOGEXCEPT("\"{0}\" does not exist.", file);
	}
	std::filesystem::path fileInfo(file);
	if (!fileInfo.has_extension()) {
		LOGEXCEPT("\"{0}\" has no file extention; cannot deduce shader type.", file);
	}

	std::vector<egx::internal::ShaderCacheInformation> cacheList;
	if (!defines.HasValue() && !compileDebug && Shader2::CacheFile.has_value()) {
		cacheList = Shader2::LoadCacheInformation();
		auto fullPath = fileInfo.string();
		egx::internal::ShaderCacheInformation cache;
		if (!CheckIfShaderHasBeenModified(cacheList, fullPath, cache)) {
			return CreateFromCache(CoreInterface, cache, Attributes);
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
		LOGEXCEPT("\"{0}\" has unknown extension '{1}'. Supported extensions are .vert, .frag, and .comp", file, extension);
	}

	auto code = ReadAllText(file);
	if (!code.has_value()) {
		LOGEXCEPT("Could not load file \"{0}\"", file);
	}
	std::vector<std::pair<std::string, uint64_t>> includeLastModified;
	auto processedCode = PreprocessInclude(fileInfo.string(), std::filesystem::absolute(fileInfo).parent_path().string(), *code, includeLastModified);
	auto result = FactoryCreateEx(CoreInterface, processedCode, shaderStage, Attributes, std::filesystem::absolute(fileInfo).string(), fileInfo.filename().string(), defines, compileDebug);

	if (!defines.HasValue() && !compileDebug && Shader2::CacheFile.has_value()) {
		internal::ShaderCacheInformation info;
		info.FilePath = file;
		info.LastModifiedDate = std::filesystem::last_write_time(fileInfo).time_since_epoch().count();
		info.ShaderStage = shaderStage;
		info.Bytecode = result->Bytecode;
		info.IncludesLastModified = includeLastModified;
		if (cacheList.size() > 0)
			cacheList.erase(std::remove_if(cacheList.begin(), cacheList.end(), [&](auto& x) {return x.FilePath == info.FilePath; }), cacheList.end());
		cacheList.push_back(info);
		SaveCacheInformation(cacheList);
	}
}

ref<Shader2> egx::Shader2::FactoryCreateEx
(const ref<VulkanCoreInterface>& CoreInterface, std::string_view Code, VkShaderStageFlags ShaderType,
	BindingAttributes Attributes,
	std::string_view SourceCodeFilePath,
	std::string_view OptionalFileName,
	Shader2Defines defines,
	bool compileDebug)
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
	options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
	// Supported by Vulkan 1.2
	options.SetTargetSpirv(shaderc_spirv_version_1_5);

	bool debug = compileDebug;
#ifdef _DEBUG
	debug |= true;
#endif
	options.SetWarningsAsErrors();
	if (debug) {
		options.SetOptimizationLevel(shaderc_optimization_level_zero);
		options.SetGenerateDebugInfo();
	}
	else {
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
	}

	std::string defineList;
	for (auto& [preprocessor, value] : defines.Defines)
	{
		defineList += cpp::Format("#define {0} {1}\n", preprocessor, value);
	}
	// defines must be after #version
	auto versionIndex = cpp::IndexOf(Code, "#version");
	std::string_view code_without_version = Code.substr(versionIndex);
	code_without_version = code_without_version.substr(cpp::IndexOf(code_without_version, "\n"));
	std::string_view version = Code.substr(versionIndex, code_without_version.data() - Code.data());
	std::stringstream code_stream;
	code_stream << version << '\n' << defineList << code_without_version;
	auto code = code_stream.str();

	auto start = std::chrono::high_resolution_clock::now();
	auto compilationResult = compiler.CompileGlslToSpv(code, shaderKind, OptionalFileName.data(), "main", options);
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	if (compilationResult.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success)
	{
		if (OptionalFileName.length() > 0) {
			LOGEXCEPT("Encountered error during compilation.\nFile:{0}\n{1}", OptionalFileName.data(), compilationResult.GetErrorMessage());
		}
		LOGEXCEPT("Encountered error during compilation.\n{1}", compilationResult.GetErrorMessage());
	}
	if (compilationResult.GetNumWarnings() > 0) {
		if (OptionalFileName.length() > 0) {
			LOG(WARNING, "{0} Warnings for Shader \"{1}\"", compilationResult.GetNumWarnings(), OptionalFileName.data());
		}
	}
	auto bytecode = std::vector<uint32_t>(compilationResult.begin(), compilationResult.end());
	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = bytecode.size() * sizeof(uint32_t);
	createInfo.pCode = bytecode.data();
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(CoreInterface->Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		LOGEXCEPT("Could not create shader module for '{0}'.", OptionalFileName.data());
	}
	auto reflection = GenerateReflection(bytecode, Attributes);
	auto result = new Shader2(ShaderType, duration, bytecode, shaderModule, Attributes, reflection, shaderKind, CoreInterface, SourceCodeFilePath.data());
	if (SourceCodeFilePath.size() <= 0 || !LazyLoadShaderSourceCode || defines.HasValue()) {
		result->_SourceCode = code;
	}
	return result;
}

std::string& egx::Shader2::GetSourceCode()
{
	if (_SourceCode.size() > 0) return _SourceCode;
	std::vector<std::pair<std::string, uint64_t>> includeLastModified;
	try {
		auto code = cpp::ReadAllText(_SourceFilePath);
		if (!code.has_value()) {
			LOG(ERR, "Could not GetSourceCode() because {0} could not read. (Shader was loaded using LazyLoad source code)", _SourceFilePath);
		}
		_SourceCode = PreprocessInclude(_SourceFilePath, std::filesystem::absolute(_SourceFilePath).parent_path().string(), *code, includeLastModified);
	}
	catch (std::exception e)
	{
		LOG(ERR, e.what());
	}
	return _SourceCode;
}

std::string egx::Shader2::PreprocessInclude(std::string_view CurrentFilePath, std::string_view SourceDirectory, std::string_view code,
	std::vector<std::pair<std::string, uint64_t>>& includesLastModifiedDate, std::optional<std::vector<std::string>> pragmaOnce)
{
	if (!pragmaOnce.has_value()) {
		pragmaOnce = std::vector<std::string>();
	}
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
		include_file_path = (std::filesystem::path(SourceDirectory) / include_file).string();
		if (!std::filesystem::exists(include_file_path)) {
			LOGEXCEPT("#include is not found in \"{0}\" at line {1}", CurrentFilePath.data(), line_index);
		}
		// did we already include the file
		if (!std::ranges::any_of(pragmaOnce.value(), [&](std::string& t) {return t == include_file_path; })) {

			auto include_source_code = ReadAllText(include_file_path);
			if (!include_source_code.has_value()) {
				LOGEXCEPT("Could not open #include file (READ FAIL) in \"{0}\" at line {1}", CurrentFilePath.data(), line_index);
			}
			std::filesystem::path p(include_file_path);
			includesLastModifiedDate.push_back({ p.string(), (uint64_t)std::filesystem::last_write_time(p).time_since_epoch().count() });
			include_source_code = PreprocessInclude(include_file_path, p.parent_path().string(), *include_source_code, includesLastModifiedDate, pragmaOnce);
			output += *include_source_code;
			pragmaOnce->push_back(cpp::UpperCase(include_file_path));
		}
		output.push_back('\n');
		line_index++;
	}
	return output;
}

void Shader2::SetGlobalCacheFile(std::string_view CacheFolder)
{
	std::lock_guard guard(Shader2::CacheWriteLock);
	if (std::filesystem::is_directory(CacheFolder) || std::filesystem::create_directories(CacheFolder)) {
		const char* mode = nullptr;
#ifdef _DEBUG
		mode = "DEBUG";
#else
		mode = "RELEASE";
#endif
		std::filesystem::path path = std::filesystem::path(CacheFolder) / Format("{0}_{1}{2}", CacheFileName, mode, CacheFileExtension);
		//sqlite3_open(path.string().c_str(), &GlobalDb);
		//if (!GlobalDb) {
		//	LOG(ERR, "Could not create cache database.");
		//	return;
		//}
		//std::string sql = R"(CREATE TABLE IF NOT EXISTS CacheRegistry (
		//			Id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
		//			FilePath TEXT NOT NULL,
		//			LastModified INTEGER NOT NULL,
		//			ByteCode BLOB NOT NULL,
		//			SourceCode TEXT NOT NULL,
		//			Defines TEXT NOT NULL
		//	))";

		//char* pErrorMessage = nullptr;
		//if (sqlite3_exec(GlobalDb, sql.c_str(), nullptr, nullptr, &pErrorMessage) != SQLITE_OK) {
		//	LOG(ERR, "SQLITE3 Create Table Error {0}", pErrorMessage);
		//	sqlite3_free(pErrorMessage);
		//	sqlite3_close(GlobalDb);
		//	GlobalDb = nullptr;
		//}
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
	ordered_json j;
	try {
		j = ordered_json::parse(*cache);
		std::vector< internal::ShaderCacheInformation> result;
		for (auto& item : j) {
			auto& p = result.emplace_back();
			item.at("filePath").get_to(p.FilePath);
			item.at("shaderStage").get_to(p.ShaderStage);
			item.at("byteCode").get_to(p.Bytecode);
			item.at("lastModifiedDate").get_to(p.LastModifiedDate);
			auto& includeLastModified = item.at("includeLastModified");
			for (auto& ilm : includeLastModified)
			{
				auto& includeDirective = p.IncludesLastModified.emplace_back();
				ilm.at("includePath").get_to<std::string>(includeDirective.first);
				ilm.at("lastModified").get_to<uint64_t>(includeDirective.second);
			}
		}
		return result;
	}
	catch (...) {
		return {};
	}
}

void Shader2::SaveCacheInformation(const std::vector<internal::ShaderCacheInformation>& cacheInformation)
{
	if (!Shader2::CacheFile.has_value() || Shader2::CacheFile->length() == 0) return;
	std::lock_guard guard(Shader2::CacheWriteLock);
	ordered_json out = json::array();
	for (auto& item : cacheInformation) {
		ordered_json includeLastModified;
		for (auto& [path, lastModified] : item.IncludesLastModified)
		{
			includeLastModified.push_back({
				{"includePath", path},
				{"lastModified", lastModified}
				});
		}
		out.push_back({
			{"filePath", item.FilePath},
			{"shaderStage", item.ShaderStage},
			{"byteCode", item.Bytecode},
			{"lastModifiedDate", item.LastModifiedDate},
			{ "includeLastModified", includeLastModified }
			});
	}
	WriteAllText(*Shader2::CacheFile, out.dump());
}

internal::ShaderCacheInformation egx::Shader2::LoadCacheInformationDb(const std::string& filePath)
{
	//char* pErrorMsg = nullptr;
	//sqlite3_exec(GlobalDb, cpp::Format("SELECT * FROM CacheRegistry WHERE FilePath='{0}'", filePath).c_str(), [](void*, int argc, char** argv, char** column) -> int {
	//	
	//	return SQLITE_ABORT;
	//}, nullptr, &pErrorMsg);
	//return internal::ShaderCacheInformation();
	return {};
}

void egx::Shader2::SaveCacheInformationDb(const std::vector<internal::ShaderCacheInformation>& cacheInformation)
{
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
	readImageResources(resources.subpass_inputs, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
	// (TODO) Read Samplers?
	//readImageResources(resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER);

	return output;
}

// return true if modified otherwise false
bool egx::Shader2::CheckIfShaderHasBeenModified(const std::vector<internal::ShaderCacheInformation>& cacheInformation, const std::string& shaderPath, egx::internal::ShaderCacheInformation& outCache)
{
	auto cache = std::ranges::find_if(cacheInformation, [&](const internal::ShaderCacheInformation& info) { return EqualIgnoreCase(shaderPath, info.FilePath); });
	if (cache != cacheInformation.end()) {
		if (std::filesystem::last_write_time(shaderPath).time_since_epoch().count() == cache->LastModifiedDate) {
			for (auto& ilm : cache->IncludesLastModified) {
				if (std::filesystem::last_write_time(ilm.first).time_since_epoch().count() != ilm.second) {
					return true;
				}
			}
			outCache = *cache;
			return false;
		}
	}
	return true;
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
	return new Shader2(CacheInfo.ShaderStage, 0ull, CacheInfo.Bytecode, shaderModule, Attributes, reflection, shaderKind, CoreInterface, CacheInfo.FilePath);
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