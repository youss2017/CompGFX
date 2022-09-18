#include "egxshader.hpp"
#include <spirv_cross/spirv_glsl.hpp>
#include <chrono>
#include <random>
#include <util/utilcore.hpp>
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

	EGX_API egxshader::egxshader(ref<VulkanCoreInterface> CoreInterface, const char* __ShaderPath, const char* EntryPointFunction)
		: CoreInterface(CoreInterface), m_EntryPointFunction(EntryPointFunction)
	{
		std::filesystem::path ShaderPath;
		try {
			ShaderPath = std::filesystem::canonical(ut::replace(std::string(__ShaderPath), "\\", "/"));
		}
		catch (std::exception& e) {
			std::string error_message = "Could not load shader file '" + std::string(__ShaderPath) + "' because: " + e.what();
			logerrors(error_message);
			assert(0);
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
		extension = ut::lower(extension);
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
			ut::log_error("Unsupported shader extension or an invalid shader extension. (Support shader extensions are .vert, .frag, .comp)", __FILE__, __LINE__);
			assert(0);
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
				logalert("Could not load SPIR-V Cache even though it was listed in SPIR-V Cache catalog.");
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
				std::string err = std::string(ShaderPath.string()) + " --- Failed to compile into SPIR-V.\n" + spv.GetErrorMessage();
				// TODO: The following steps! but is this necessary?
				// 1) Remove from catalog
				// 2) Delete from cache!
				ut::log_error(err.c_str(), __FILE__, 0);
				throw std::runtime_error(err);
			}
			else
			{
				std::string info = shader_path.string() + " compiled successfully!";
				loginfo(info.c_str());
			}

			m_Bytecode = std::vector<uint32_t>(spv.cbegin(), spv.cend());
			double duration = (double)(std::chrono::high_resolution_clock::now() - start).count();
			duration *= 1e-6;
			std::stringstream ss;
			ss << "Compilation for " << shader_path.filename().string() << " took " << duration << " ms.";
			logalerts(ss.str());
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
				std::vector<std::string> old_catalog = ut::split(&temp[1], "\n");
				std::string new_catalog;
				// Modify Catalog to store new data
				bool modified = false;
				for (auto& item : old_catalog)
				{
					if (item.length() <= 1)
						continue;
					item = ut::trim(item);
					std::vector<std::string> item_split = ut::split(item, "::");
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
					std::string alert_message = "Could not cache shader bytecode for " + spirv_file_path;
					logalert(alert_message.c_str());
				}
			}
		}

		InitalizeVulkan();
	}

	EGX_API egxshader::egxshader(egxshader&& move)
	{
		memcpy(this, &move, sizeof(egxshader));
		memset(&move, 0, sizeof(egxshader));
	}

	EGX_API egxshader& egxshader::operator=(egxshader& move)
	{
		memcpy(this, &move, sizeof(egxshader));
		memset(&move, 0, sizeof(egxshader));
		return *this;
	}

	std::string egxshader::ProcessIncludeDirectives(std::string source_code, std::string root_directory)
	{
		std::string full_code;

		std::vector<std::string> sl = ut::split(source_code, "\n");
		for (size_t i = 0; i < sl.size(); i++)
		{
			auto str = ut::ltrim(sl[i]);
			if (ut::StartsWith(str, "#include"))
			{
				// get file to include
				std::string include_file = str.substr(9);
				include_file = ut::replaceAll(include_file, "\"", "");
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
					std::string error_msg = std::string("Could not get #include directive file full path, specifically [") + std::string(include_file) + std::string("], Caught Exception:\n\n") + std::string(e.what()) + "\n";
					ut::log_error(error_msg.c_str(), __FILE__, __LINE__);
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

	void egxshader::InitalizeVulkan()
	{
		VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = m_Bytecode.size() * 4u;
		createInfo.pCode = this->GetBytecode();
		VkShaderModule m;
		vkCreateShaderModule(CoreInterface->Device, &createInfo, 0, &m);
		m_ShaderHandle = m;
	}

	void egxshader::EvaluateCaching(std::filesystem::path shader_path, bool& UsingCache, bool& Cached, std::string& identifier)
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
					std::vector<std::string> catalog = ut::split(catalog_string, "\n");
					std::string last_access = std::to_string(uint64_t(std::filesystem::last_write_time(shader_path).time_since_epoch().count()));
					for (auto& subject : catalog)
					{
						subject = ut::trim(subject);
						if (subject.length() <= 1)
							continue;
						std::vector<std::string> subcomponents = ut::split(subject, "::");
						if (subcomponents.size() != 5)
						{
							ut::log_error("SPIRV Catalog has been corrupted! Deleting catalog!", __FILE__, __LINE__);
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
										cache_result_info = "Cache match for " + shader_path.string();
										loginfo(cache_result_info.c_str());
									}
									else
									{
										cache_result_info = "Cache mismatch for " + shader_path.string();
										logalert(cache_result_info.c_str());
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
					std::string catalog_create_alert = "Creating SPIR-V Caching Catalog: " + std::string(SPIRV_CATALOG);
					logalerts(catalog_create_alert);
					ut::CreateEmptyFile(catalog_path.string());
				}
			}
			else
			{
				std::string create_alert = "Creating SPIR-V Caching Directory Hierarchy: " + s_CacheDirectory;
				logalerts(create_alert);
				std::filesystem::create_directories(s_CacheDirectory);
				// create catalog
				std::string catalog_create_alert = "Creating SPIR-V Caching Catalog: " + std::string(SPIRV_CATALOG);
				logalerts(catalog_create_alert);
				ut::CreateEmptyFile(catalog_path.string());
			}
		}
	}

	EGX_API egxshader::~egxshader()
	{
		vkDestroyShaderModule(CoreInterface->Device, (VkShaderModule)m_ShaderHandle, 0);
	}

	EGX_API const std::string& egxshader::GetSource()
	{
		if (m_Source.size() > 0)
			return m_Source;
		// otherwise the source code was not loaded, probably the cached bytecode was loaded.
		m_Source = &ut::ReadTextFile(m_ShaderPath)[1];
		m_Source = this->ProcessIncludeDirectives(m_Source, m_ShaderDirectory);
		return m_Source;
	}

	void EGX_API egxshader::CompileVulkanSPIRVText(const char* source_code, const char* filename, shaderc_shader_kind shader_type, uint32_t** pOutCode, uint32_t* pOutSize, const char* EntryPointFunction)
	{
		assert(pOutCode && pOutSize);

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		options.SetTargetSpirv(SPIRVCompilationLevel);

		assert(compiler.IsValid());

		auto spv = compiler.CompileGlslToSpv(source_code, strlen(source_code), shader_type, filename, EntryPointFunction, options);
		if (spv.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			ut::log_error(spv.GetErrorMessage().c_str(), __FILE__, __LINE__);
			ut::DebugBreak();
		}

		std::vector<uint32_t> bytecode = std::vector<uint32_t>(spv.cbegin(), spv.cend());
		uint32_t* pBytecode = new uint32_t[bytecode.size()];
		memcpy(pBytecode, bytecode.data(), bytecode.size() * sizeof(uint32_t));

		*pOutCode = pBytecode;
		*pOutSize = bytecode.size() * 4;
	}

	std::string egxshader::s_CacheDirectory;
	EGX_API bool egxshader::ConfigureShaderCache(std::string SpirvCahceFolder)
	{
		if (SpirvCahceFolder.size() == 0)
		{
			egxshader::s_CacheDirectory = "";
			return true;
		}
		SpirvCahceFolder = ut::replaceAll(SpirvCahceFolder, "\\", "/");

		if (!std::filesystem::exists(SpirvCahceFolder))
		{
			std::string warning = "SPIR-V Cahce Directory does not exist, attempting to create directory hierarchy: " + SpirvCahceFolder;
			logwarnings(warning);
			try
			{
				std::filesystem::create_directories(SpirvCahceFolder);
			}
			catch (std::exception& e)
			{
				std::string alert_message = std::string("Could not create SPIRV-Cahce Directory! SPIR-V Caching is not ENABLED!") + e.what();
				logalerts(alert_message);
				egxshader::s_CacheDirectory = "";
				return false;
			}
		}
		egxshader::s_CacheDirectory = std::filesystem::canonical(SpirvCahceFolder).string();
		std::string info_cache = "Shader Cache Directory set to: " + egxshader::s_CacheDirectory;
		loginfos(info_cache);
		return true;
	}

}