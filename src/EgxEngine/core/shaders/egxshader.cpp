#include "egxshader.hpp"
#include <spirv_cross/spirv_glsl.hpp>
#include <chrono>
#include <random>
#include <Utility/CppUtility.hpp>
#include <sstream>

#ifdef _DEBUG
constexpr bool OptimizeShaders = false;
#else
constexpr bool OptimizeShaders = true;
#endif
constexpr bool PrintGLSLShader = true;
constexpr shaderc_spirv_version SPIRVCompilationLevel = shaderc_spirv_version_1_5;

/* SPIR-V Catalog Format
	[File Directory]::[identifier]::[Filename Stem]::[Filename Extension]::[Last Accessed Data]
*/
// Format: [identifier].[Shader Filename].[Shader Extension].[SPIRV_EXTENSION]
constexpr const char* SPIRV_EXTENSION = ".spv";
constexpr const char* SPIRV_CATALOG = "spirv_caching_catalog.txt";

static std::string Shader_Internal_GenerateIdentifier()
{
	auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() + std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937_64 random_engine(time);
	return std::to_string(random_engine());
}

namespace egx {

	EGX_API Shader::Shader(ref<VulkanCoreInterface> CoreInterface, const char* __ShaderPath, const char* EntryPointFunction)
		: CoreInterface(CoreInterface), m_EntryPointFunction(EntryPointFunction)
	{
		std::filesystem::path ShaderPath;
		try {
			ShaderPath = std::filesystem::canonical(ut::Replace(std::string(__ShaderPath), "\\", "/"));
		}
		catch (std::exception& e) {
			LOG(ERR, "Could not load shader source file '{0}' because: {1}", __ShaderPath, e.what());
			throw std::runtime_error("Shader source not found.");
		}
		std::filesystem::path shader_path(ShaderPath.string());
		m_ShaderPath = shader_path.string();
		assert(shader_path.has_filename() && shader_path.has_extension() && "ShaderPath format error!");

#if defined(_MSC_VER)
		std::wstring ShaderDirectory = shader_path.parent_path().c_str();
		std::wstring ShaderFilename = shader_path.filename().c_str();
		m_ShaderDirectory = ut::ToAsciiString(ShaderDirectory);
		m_ShaderFilename = ut::ToAsciiString(ShaderFilename);
#else
		std::string ShaderDirectory = shader_path.parent_path().c_str();
		std::string ShaderFilename = shader_path.filename().c_str();
		m_ShaderDirectory = ShaderDirectory;
		m_ShaderFilename = ShaderFilename;
#endif

		// Check Cache!
		bool UseCaching = false;
		bool Cached = false;
		std::string identifier;
		this->EvaluateCaching(shader_path, UseCaching, Cached, identifier);

#if defined(_MSC_VER)
		std::wstring extension = shader_path.extension().c_str();
#else
		std::string ascii_extension_str = shader_path.extension().c_str();
		std::wstring extension = Utils::StringToWideString(ascii_extension_str);
#endif
		extension = ut::LowerCase(extension);
		shaderc_shader_kind shader_type;
		if (extension.compare(L".vert") == 0)
		{
			shader_type = shaderc_shader_kind::shaderc_glsl_vertex_shader;
		}
		else if (extension.compare(L".frag") == 0)
		{
			shader_type = shaderc_shader_kind::shaderc_glsl_fragment_shader;
		}
		else if (extension.compare(L".comp") == 0)
		{
			shader_type = shaderc_shader_kind::shaderc_glsl_compute_shader;
		}
		else
		{
			LOG(ERR, "Unsupported shader extension or an invalid shader extension. (Support shader extensions are .vert, .frag, .comp)");
			throw std::runtime_error("Unknown shader extension.");
		}
		m_ShaderKind = shader_type;

		bool BytecodeLoaded = false;
		if (Cached)
		{
			// Load cache
			// Format: [identifier].[Shader Filename].[Shader Extension].[SPIRV_EXTENSION]
			std::filesystem::path spv_cache(s_CacheDirectory + "/" + identifier + "." + shader_path.stem().string() + "." + shader_path.extension().string() + "." + SPIRV_EXTENSION);
			if (std::filesystem::exists(spv_cache))
			{
				auto bytecode8 = ut::ReadBinaryFile(spv_cache.string());
				if (bytecode8[0] == 0 || (bytecode8.size() - 1) % 4 != 0) {
					BytecodeLoaded = false;
				}
				else {
					m_Bytecode.resize((bytecode8.size() - 1u) / 4u);
					memcpy(&m_Bytecode[0], &bytecode8[1], bytecode8.size() - 1);
					BytecodeLoaded = true;
				}
			}
			else
			{
				LOG(INFOBOLD, "Could not load SPIR-V Cache even though it was listed in SPIR-V Cache catalog.");
				BytecodeLoaded = false;
			}
		}

		if (!BytecodeLoaded)
		{
			auto start = std::chrono::high_resolution_clock::now();
			auto loadedSource = ut::ReadTextFile(ShaderPath.string());
			if (loadedSource[0] == 0) {
				using namespace std;
				throw std::runtime_error("Could not load shader "s + ShaderPath.string());
			}
			m_Source = &loadedSource[1];
			m_Source = this->ProcessIncludeDirectives(m_Source, m_ShaderDirectory);
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
			options.SetOptimizationLevel(OptimizeShaders ? shaderc_optimization_level_performance : shaderc_optimization_level_zero);
			options.SetAutoBindUniforms(true);
			options.SetAutoSampledTextures(true);
			options.SetTargetSpirv(SPIRVCompilationLevel);

			assert(compiler.IsValid());
			auto spv = compiler.CompileGlslToSpv(m_Source.c_str(), m_Source.length(), shader_type, m_ShaderFilename.c_str(), m_EntryPointFunction.c_str(), options);
			m_CompileStatus = spv.GetCompilationStatus() == shaderc_compilation_status_success;

			if (!m_CompileStatus)
			{
				// TODO: The following steps! but is this necessary?
				// 1) Remove from catalog
				// 2) Delete from cache!
				LOG(ERR, "{0} --- Failed to compile into SPIR-V.", ShaderPath.string(), spv.GetErrorMessage());
				throw std::runtime_error("SPIR-V Compilation Failed.");
			}
			else
			{
				LOG(INFO, "{0} compiled successfully.", shader_path.filename().string());
			}

			m_Bytecode = std::vector<uint32_t>(spv.cbegin(), spv.cend());
			double duration = (double)(std::chrono::high_resolution_clock::now() - start).count();
			duration *= 1e-6;
			LOG(INFOBOLD, "Compilation for {0} took {1} ms.", shader_path.filename().string(), duration);
			if (UseCaching)
			{
				identifier = Shader_Internal_GenerateIdentifier();
				std::string last_access = std::to_string(uint64_t(std::filesystem::last_write_time(shader_path).time_since_epoch().count()));

				/* SPIR-V Catalog Format
					[File Directory]::[identifier]::[Filename Stem]::[Filename Extension]::[Last Accessed Data]
				*/
				// Format: [identifier].[Shader Filename].[Shader Extension].[SPIRV_EXTENSION]
				// 1) Update Catalog
				std::string catalog_item = m_ShaderDirectory + "::" + identifier + "::" + shader_path.stem().string() + "::" + shader_path.extension().string() + "::" + last_access;
				// Load Old Catalog

				/* Create Catalog File if it does not exist */
				if (!ut::Exists(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG)))
				{
					ut::CreateEmptyFile(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG));
				}

				auto temp = ut::ReadTextFile(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG));
				std::vector<std::string> old_catalog = ut::Split(&temp[1], "\n");
				std::string new_catalog;
				// Modify Catalog to store new data
				bool modified = false;
				for (auto& item : old_catalog)
				{
					if (item.length() <= 1)
						continue;
					item = ut::Trim(item);
					std::vector<std::string> item_split = ut::Split(item, "::");
					assert(item_split.size() == 5);
					if (item_split[0].compare(m_ShaderDirectory) == 0 && item_split[2].compare(shader_path.stem().string()) == 0 && item_split[3].compare(shader_path.extension().string()) == 0)
					{
						modified = true;
						std::string spirv_filename = item_split[1] + "." + shader_path.stem().string() + "." + shader_path.extension().string() + "." + std::string(SPIRV_EXTENSION);
						std::string spirv_file_path = std::filesystem::path(s_CacheDirectory + "/" + spirv_filename).make_preferred().string();
						std::filesystem::remove(spirv_file_path);
						item = catalog_item;
						break;
					}
					new_catalog += item + "\n";
				}
				new_catalog += catalog_item;

				// Override old catalog with new data
				ut::WriteTextFile(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG), new_catalog.c_str(), new_catalog.size());
				// 2) Store SPIR-V bytecode
				std::string spirv_filename = identifier + "." + shader_path.stem().string() + "." + shader_path.extension().string() + "." + std::string(SPIRV_EXTENSION);
				std::string spirv_file_path = std::filesystem::path(s_CacheDirectory + "/" + spirv_filename).make_preferred().string();
				if (!ut::WriteBinaryFile(spirv_file_path, m_Bytecode.data(), m_Bytecode.size() * 4u))
				{
					LOG(WARNING, "Could not cache shader bytecode for {0}", spirv_file_path);
				}
			}
		}

		InitalizeVulkan();
	}

	EGX_API Shader::Shader(Shader&& move)
	{
		memcpy(this, &move, sizeof(Shader));
		memset(&move, 0, sizeof(Shader));
	}

	EGX_API Shader& Shader::operator=(Shader& move)
	{
		memcpy(this, &move, sizeof(Shader));
		memset(&move, 0, sizeof(Shader));
		return *this;
	}

	std::string Shader::ProcessIncludeDirectives(std::string source_code, std::string root_directory)
	{
		std::string full_code;

		std::vector<std::string> sl = ut::Split(source_code, "\n");
		for (size_t i = 0; i < sl.size(); i++)
		{
			auto str = ut::LTrim(sl[i]);
			if (ut::StartsWith(str, "#include"))
			{
				// get file to include
				std::string include_file = str.substr(9);
				include_file = ut::ReplaceAll(include_file, "\"", "");
				std::string include_path;
				if (include_file[0] != '/' && include_file[0] != '\\')
				{
					include_path = std::filesystem::path(root_directory + "/" + include_file).make_preferred().string();
				}
				else
				{
					include_path = root_directory + include_file;
				}
				std::filesystem::path correct_path;
				try
				{
					correct_path = std::filesystem::canonical(include_path);
				}
				catch (std::exception& e)
				{
					LOG(ERR, "Could not get #include directive file full path, specifically [{0}], Caught Exception:\n{1}", include_file, e.what());
					return source_code;
				}
				auto includeSource = ut::ReadTextFile(correct_path.string());
				if (includeSource[0] == 0) {
					using namespace std;
					throw std::runtime_error("Could not load #include file "s + correct_path.string());
				}
				std::string include_source_code = this->ProcessIncludeDirectives(&includeSource[1], correct_path.root_directory().string());
				full_code += include_source_code + "\n";
			}
			else
			{
				full_code += sl[i] + "\n";
			}
		}

		return full_code;
	}

	void Shader::InitalizeVulkan()
	{
		VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = m_Bytecode.size() * 4u;
		createInfo.pCode = m_Bytecode.data();
		VkShaderModule m;
		vkCreateShaderModule(CoreInterface->Device, &createInfo, 0, &m);
		m_ShaderHandle = m;
	}

	void Shader::EvaluateCaching(std::filesystem::path shader_path, bool& UsingCache, bool& Cached, std::string& identifier)
	{
		// Check Cache!
		UsingCache = false, Cached = false, identifier = "";
		std::filesystem::path catalog_path(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG));
		if (s_CacheDirectory.size() > 0)
		{
			UsingCache = true;
			if (std::filesystem::exists(s_CacheDirectory))
			{
				// check catalog!
				if (std::filesystem::exists(catalog_path))
				{
					/* SPIR-V Catalog Format
						[File Directory]::[identifier]::[Filename Stem]::[Filename Extension]::[Last Accessed Data]
					*/
					std::string catalog_string = &ut::ReadTextFile(catalog_path.string())[1];
					std::vector<std::string> catalog = ut::Split(catalog_string, "\n");
					std::string last_access = std::to_string(uint64_t(std::filesystem::last_write_time(shader_path).time_since_epoch().count()));
					for (auto& subject : catalog)
					{
						subject = ut::Trim(subject);
						if (subject.length() <= 1)
							continue;
						std::vector<std::string> subcomponents = ut::Split(subject, "::");
						if (subcomponents.size() != 5)
						{
							LOG(ERR, "SPIRV Catalog has been corrupted; deleting catalog.");
							std::filesystem::remove(catalog_path);
							break;
						}

						if (subcomponents[0].compare(shader_path.parent_path().string()) == 0)
						{
							if (subcomponents[2].compare(shader_path.stem().string()) == 0)
							{
								if (subcomponents[3].compare(shader_path.extension().stem().string()) == 0)
								{
									// compare time
									std::string cache_result_info;
									if (subcomponents[4].compare(last_access) == 0)
									{
										Cached = true;
										LOG(INFO, "Cache match for {0}", shader_path.string());
									}
									else
									{
										LOG(INFOBOLD, "Cache mismatch for {0}", shader_path.string());
									}
									identifier = subcomponents[1];
									break;
								}
							}
						}
					}
				}
				else
				{
					// create catalog
					LOG(INFOBOLD, "Creating SPIR-V Caching Catalog: {0}", SPIRV_CATALOG);
					ut::CreateEmptyFile(catalog_path.string());
				}
			}
			else
			{
				LOG(INFOBOLD, "Creating SPIR-V Caching Directory Hierarchy: {0}", s_CacheDirectory);
				std::filesystem::create_directories(s_CacheDirectory);
				// create catalog
				LOG(INFOBOLD, "Creating SPIR-V Caching Catalog: {0}", SPIRV_CATALOG);
				ut::CreateEmptyFile(catalog_path.string());
			}
		}
	}

	EGX_API Shader::~Shader()
	{
		if (m_ShaderHandle)
			vkDestroyShaderModule(CoreInterface->Device, (VkShaderModule)m_ShaderHandle, 0);
	}

	EGX_API const std::string& Shader::GetSource()
	{
		if (m_Source.size() > 0)
			return m_Source;
		// otherwise the source code was not loaded, probably the cached bytecode was loaded.
		m_Source = &ut::ReadTextFile(m_ShaderPath)[1];
		m_Source = this->ProcessIncludeDirectives(m_Source, m_ShaderDirectory);
		return m_Source;
	}

	void EGX_API Shader::CompileVulkanSPIRVText(const char* source_code, const char* filename, shaderc_shader_kind shader_type, std::vector<uint32_t>& OutCode, const char* EntryPointFunction)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		options.SetTargetSpirv(SPIRVCompilationLevel);

		assert(compiler.IsValid());

		auto spv = compiler.CompileGlslToSpv(source_code, strlen(source_code), shader_type, filename, EntryPointFunction, options);
		if (spv.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			LOG(ERR, spv.GetErrorMessage());
			ut::DebugBreak();
		}

		OutCode = std::vector<uint32_t>(spv.cbegin(), spv.cend());
	}

    Shader Shader::CreateFromSource(const ref<VulkanCoreInterface>& CoreInterface, const char* source_code, shaderc_shader_kind shader_type)
	{
		Shader ret;
		ret.CoreInterface = CoreInterface;
		ret.m_Source = source_code;
		CompileVulkanSPIRVText(source_code, "main.glsl", shader_type, ret.m_Bytecode);
		ret.m_CompileStatus = true;
		ret.InitalizeVulkan();
		return ret;
	}


	std::string Shader::s_CacheDirectory;
	EGX_API bool Shader::ConfigureShaderCache(std::string SpirvCahceFolder)
	{
		if (SpirvCahceFolder.size() == 0)
		{
			Shader::s_CacheDirectory = "";
			return true;
		}
		SpirvCahceFolder = ut::ReplaceAll(SpirvCahceFolder, "\\", "/");

		if (!std::filesystem::exists(SpirvCahceFolder))
		{
			LOG(WARNING, "SPIR-V Cahce Directory does not exist, attempting to create directory hierarchy: {0}", SpirvCahceFolder.c_str());
			try
			{
				std::filesystem::create_directories(SpirvCahceFolder);
			}
			catch (std::exception& e)
			{
				LOG(ERR, "Could not create SPIRV-Cahce Directory. SPIR-V Caching is not enabled -- {0}", e.what());
				Shader::s_CacheDirectory = "";
				return false;
			}
		}
		Shader::s_CacheDirectory = std::filesystem::canonical(SpirvCahceFolder).string();
		LOG(INFO, "Shader Cache Directory set to: {0}", Shader::s_CacheDirectory);
		return true;
	}

}