#include "Shader.hpp"
#include "../../utils/Logger.hpp"
#include "../../utils/common.hpp"
#include "../backend/VkGraphicsCard.hpp"
#include "../../utils/StringUtils.hpp"
#include <spirv_cross/spirv_glsl.hpp>
#include <chrono>
#include <random>

#ifdef _DEBUG
constexpr bool OptimizeShaders = false;
#else
constexpr bool OptimizeShaders = true;
#endif
constexpr bool PrintGLSLShader = false;
constexpr shaderc_spirv_version SPIRVCompilationLevel = shaderc_spirv_version_1_5;

/* SPIR-V Catalog Format
    [File Directory]::[identifier]::[Filename Stem]::[Filename Extension]::[Last Accessed Data]
*/
// Format: [identifier].[Shader Filename].[Shader Extension].[SPIRV_EXTENSION]
constexpr const char *SPIRV_EXTENSION = ".spv";
constexpr const char *SPIRV_CATALOG = "spirv_caching_catalog.txt";

static std::string Shader_Internal_GenerateIdentifier()
{
    auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() + std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937_64 random_engine(time);
    return std::to_string(random_engine());
}

GRAPHICS_API Shader::Shader(vk::VkContext context, const char *__ShaderPath, const char *EntryPointFunction)
    : m_EntryPointFunction(EntryPointFunction)
{
    m_ApiType = (*(char*)context);
    m_Context = context;
    std::filesystem::path ShaderPath;
    try {
        ShaderPath = std::filesystem::canonical(Utils::StrReplaceAll(std::string(__ShaderPath), "\\", "/"));
    }
    catch (std::exception& e) {
        std::string error_message = "Could not load shader file '" + std::string(__ShaderPath) + "' because: " + e.what();
        logerrors(error_message);
        assert(0);
    }
    //assert(std::filesystem::exists(ShaderPath) && "Shader File Does not exist");
    std::filesystem::path shader_path(ShaderPath.string());
    m_ShaderPath = shader_path.string();
    assert(shader_path.has_filename() && shader_path.has_extension() && "ShaderPath format error!");

#if defined(_MSC_VER)
    std::wstring ShaderDirectory = shader_path.parent_path().c_str();
    std::wstring ShaderFilename = shader_path.filename().c_str();
    m_ShaderDirectory = Utils::WideStringToString(ShaderDirectory);
    m_ShaderFilename = Utils::WideStringToString(ShaderFilename);
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
    extension = Utils::LowerCaseString(extension);
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
        log_error("Unsupported shader extension or an invalid shader extension. (Support shader extensions are .vert, .frag, .comp)", __FILE__, __LINE__);
        assert(0);
    }
    m_ShaderKind = shader_type;

    int ApiType = *(char *)m_Context;

    bool BytecodeLoaded = false;
    if (Cached)
    {
        // Load cache
        // Format: [identifier].[Shader Filename].[Shader Extension].[SPIRV_EXTENSION]
        std::filesystem::path spv_cache(s_CacheDirectory + "/" + identifier + "." + shader_path.stem().string() + "." + shader_path.extension().string() + "." + SPIRV_EXTENSION);
        if (std::filesystem::exists(spv_cache))
        {
            auto bytecode8 = Utils::LoadBinaryFile(spv_cache.string().c_str());
            assert(bytecode8.size() % 4 == 0);
            m_Bytecode.resize(bytecode8.size() / 4);
            memcpy(&m_Bytecode[0], &bytecode8[0], bytecode8.size());
            BytecodeLoaded = true;
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
        m_Source = Utils::LoadTextFile(ShaderPath);
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
            log_error(err.c_str(), __FILE__, 0);
            return;
        }
        else
        {
            std::string info = shader_path.string() + " compiled successfully!";
            loginfo(info.c_str());
        }

        m_Bytecode = std::vector<uint32_t>(spv.cbegin(), spv.cend());
        double duration = (std::chrono::high_resolution_clock::now() - start).count();
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
            if (!Utils::Exists(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG)))
            {
                Utils::CreateEmptyFile(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG));
            }

            auto temp = Utils::LoadTextFile(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG));
            std::vector<std::string> old_catalog = Utils::StringSplitter("\n", temp);
            std::string new_catalog;
            // Modify Catalog to store new data
            bool modified = false;
            for (auto &item : old_catalog)
            {
                if (item.length() <= 1)
                    continue;
                item = Utils::StrTrim(item);
                std::vector<std::string> item_split = Utils::StringSplitter("::", item);
                assert(item_split.size() == 5);
                if (item_split[0].compare(m_ShaderDirectory) == 0 && item_split[2].compare(shader_path.stem().string()) == 0 && item_split[3].compare(shader_path.extension().string()) == 0)
                {
                    modified = true;
                    std::string item_full_path = item_split[0] + "/" + item_split[1] + "." + item_split[2] + "." + item_split[3] + "..spv";
                    std::filesystem::remove(item_full_path);
                    item = catalog_item;
                    break;
                }
                new_catalog += item + "\n";
            }
            if (!modified)
            {
                new_catalog += catalog_item;
            }
            // Override old catalog with new data
            Utils::WriteTextFile(s_CacheDirectory + "/" + std::string(SPIRV_CATALOG), new_catalog);
            // 2) Store SPIR-V bytecode
            std::string spirv_filename = identifier + "." + shader_path.stem().string() + "." + shader_path.extension().string() + "." + std::string(SPIRV_EXTENSION);
            std::string spirv_file_path = s_CacheDirectory + "/" + spirv_filename;
            if (!Utils::WriteBinaryFile<uint32_t>(spirv_file_path, m_Bytecode))
            {
                std::string alert_message = "Could not cache shader bytecode for " + spirv_file_path;
                logalert(alert_message.c_str());
            }
        }
    }

    InitalizeVulkan();
}

std::string Shader::ProcessIncludeDirectives(std::string source_code, std::string root_directory)
{
    std::string full_code;

    std::vector<std::string> sl = Utils::StringSplitter("\n", source_code);
    for (size_t i = 0; i < sl.size(); i++)
    {
        auto str = Utils::StrTrim(sl[i]);
        if (Utils::StrStartsWith(str, "#include"))
        {
            // get file to include
            std::string include_file = str.substr(9);
            include_file = Utils::StrRemoveAll(include_file, "\"");
            std::string include_path;
            if (include_file[0] != '/' && include_file[0] != '\\')
            {
                include_path = root_directory + "/" + include_file;
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
            catch (std::exception &e)
            {
                std::string error_msg = std::string("Could not get #include directive file full path, specifically [") + std::string(include_file) + std::string("], Caught Exception:\n\n") + std::string(e.what()) + "\n";
                log_error(error_msg.c_str(), __FILE__, __LINE__);
                return source_code;
            }
            std::string include_source_code = this->ProcessIncludeDirectives(Utils::LoadTextFile(correct_path.string().c_str()), correct_path.root_directory().string());
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
    auto context = (m_Context);
    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = this->GetByteSize();
    createInfo.pCode = this->GetBytecode();
    VkShaderModule m;
    vkcheck(vkCreateShaderModule(context->defaultDevice, &createInfo, 0, &m));
    m_ShaderHandle = m;
}

void Shader::EvaluateCaching(std::filesystem::path shader_path, bool &UsingCache, bool &Cached, std::string &identifier)
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
                std::string catalog_string = Utils::LoadTextFile(catalog_path);
                std::vector<std::string> catalog = Utils::StringSplitter("\n", catalog_string);
                std::string last_access = std::to_string(uint64_t(std::filesystem::last_write_time(shader_path).time_since_epoch().count()));
                for (auto &subject : catalog)
                {
                    subject = Utils::StrTrim(subject);
                    if (subject.length() <= 1)
                        continue;
                    std::vector<std::string> subcomponents = Utils::StringSplitter("::", subject);
                    if (subcomponents.size() != 5)
                    {
                        log_error("SPIRV Catalog has been corrupted! Deleting catalog!", __FILE__, __LINE__);
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
                Utils::CreateEmptyFile(catalog_path);
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
            Utils::CreateEmptyFile(catalog_path);
        }
    }
}

GRAPHICS_API Shader::~Shader()
{
    auto context = (m_Context);
    vkDestroyShaderModule(context->defaultDevice, (VkShaderModule)m_ShaderHandle, 0);
}

GRAPHICS_API const std::string& Shader::GetSource()
{
    if (m_Source.size() > 0)
        return m_Source;
    // otherwise the source code was not loaded, probably the cached bytecode was loaded.
    m_Source = Utils::LoadTextFile(m_ShaderPath);
    m_Source = this->ProcessIncludeDirectives(m_Source, m_ShaderDirectory);
    return m_Source;
}

void Shader::CompileVulkanSPIRVText(const char *source_code, const char *filename, shaderc_shader_kind shader_type, uint32_t **pOutCode, uint32_t *pOutSize, const char *EntryPointFunction)
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
        log_error(spv.GetErrorMessage().c_str(), __FILE__, __LINE__);
        Utils::Break();
    }

    std::vector<uint32_t> bytecode = std::vector<uint32_t>(spv.cbegin(), spv.cend());
    uint32_t *pBytecode = new uint32_t[bytecode.size()];
    memcpy(pBytecode, bytecode.data(), bytecode.size() * sizeof(uint32_t));

    *pOutCode = pBytecode;
    *pOutSize = bytecode.size() * 4;
}

std::string Shader::s_CacheDirectory;
bool Shader::ConfigureShaderCache(std::string SpirvCahceFolder)
{
    if (SpirvCahceFolder.size() == 0)
    {
        Shader::s_CacheDirectory = "";
        return true;
    }
    SpirvCahceFolder = Utils::StrReplaceAll(SpirvCahceFolder, "\\", "/");

    if (!std::filesystem::exists(SpirvCahceFolder))
    {
        std::string warning = "SPIR-V Cahce Directory does not exist, attempting to create directory hierarchy: " + SpirvCahceFolder;
        logwarnings(warning);
        try
        {
            std::filesystem::create_directories(SpirvCahceFolder);
        }
        catch (std::exception &e)
        {
            std::string alert_message = std::string("Could not create SPIRV-Cahce Directory! SPIR-V Caching is not ENABLED!") + e.what();
            logalerts(alert_message);
            Shader::s_CacheDirectory = "";
            return false;
        }
    }
    Shader::s_CacheDirectory = std::filesystem::canonical(SpirvCahceFolder).string();
    std::string info_cache = "Shader Cache Directory set to: " + Shader::s_CacheDirectory;
    loginfos(info_cache);
    return true;
}
