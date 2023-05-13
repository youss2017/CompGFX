#include "shader.hpp"
#include <spirv_cross/spirv_cross.hpp>
#include <Utility/CppUtility.hpp>
#include <filesystem>

using namespace egx;
using namespace cpp;
using namespace std;

Shader::Shader(const egx::DeviceCtx &pCtx, const std::string &file,
               BindingAttributes attributes,
               PreprocessDefines defines, bool compileDebug,
               Type overrideType)
{
    auto glsl = cpp::ReadAllText(file);
    if (!glsl.has_value())
    {
        LOGEXCEPT("Could not open {}", file);
    }
    std::filesystem::path fileInfo = file;

    Type type = overrideType;
    if (type == Type::None)
    {
        auto extension = cpp::LowerCase(fileInfo.extension().string());
        if (extension == ".vert")
        {
            type = Type::Vertex;
        }
        else if (extension == ".frag")
        {
            type = Type::Fragment;
        }
        else if (extension == ".comp")
        {
            type = Type::Compute;
        }
        else
        {
            LOGEXCEPT("Cannot determine shader type from extension {}. Supported formats: .vert, .frag, .comp", extension);
        }
    }
    m_Type = type;

    std::vector<std::pair<std::string, uint64_t>> lastModified;
    std::optional<std::vector<std::string>> pragmaOnce;
    std::string code = PreprocessIncludeFiles(file, fileInfo.parent_path().string(), glsl.value(), lastModified, pragmaOnce);
    auto start = std::chrono::high_resolution_clock::now();
    auto byteCode = CompileGlslToBytecode(code, type, defines, compileDebug, file);
    auto end = std::chrono::high_resolution_clock::now();
    m_Duration = (end - start).count() * 1e-9;
    m_SourceCode = code;

    m_Data = std::make_shared<Shader::DataWrapper>();
    m_Data->m_Ctx = pCtx;
    m_Data->m_Module = pCtx->Device.createShaderModule(vk::ShaderModuleCreateInfo({}, byteCode));

    m_Reflection = GenerateReflection(byteCode, attributes);

#ifdef _DEBUG
    LOG(INFO, "Compiled {} in {:%.4lf} ms.", file, (m_Duration * 1e3));
#endif
}

Shader::Shader(const egx::DeviceCtx &pCtx, const std::string &sourceCode,
               Shader::Type type, BindingAttributes attributes,
               PreprocessDefines defines, bool compileDebeg) : m_Type(type)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto byteCode = CompileGlslToBytecode(sourceCode, type, defines, compileDebeg, "Shader(CompileFromSource)");
    auto end = std::chrono::high_resolution_clock::now();
    m_Duration = (end - start).count() * 1e-9;
    m_SourceCode = sourceCode;

    m_Data = std::make_shared<Shader::DataWrapper>();
    m_Data->m_Ctx = pCtx;
    m_Data->m_Module = pCtx->Device.createShaderModule(vk::ShaderModuleCreateInfo({}, byteCode));

    m_Reflection = GenerateReflection(byteCode, attributes);
#ifdef _DEBUG
    LOG(INFO, "Compiled source code in {:%.4lf} ms.", (m_Duration * 1e3));
#endif
}

std::string Shader::PreprocessIncludeFiles(std::string_view CurrentFilePath,
                                           std::string_view SourceDirectory, std::string_view code,
                                           std::vector<std::pair<std::string, uint64_t>> &includesLastModifiedDate,
                                           std::optional<std::vector<std::string>> &pragmaOnce)
{
    if (!pragmaOnce.has_value())
    {
        pragmaOnce = std::vector<std::string>();
    }
    std::string output;
    auto lines = Split(code.data(), "\n");
    int line_index = 1;
    for (auto &item : lines)
    {
        if (!StartsWith(item, "#include"))
        {
            output += item;
            output.push_back('\n');
            line_index++;
            continue;
        }
        auto include_line = Split(ReplaceAll(ReplaceAll(ReplaceAll(item, "\"", ""), "<", ""), ">", ""), "\\s+");
        if (include_line.size() < 2)
        {
            LOGEXCEPT("Invalid #include in \"{0}\" at line {1}", CurrentFilePath.data(), line_index);
        }
        auto &include_file = include_line[1];
        std::filesystem::path include_file_path = (std::filesystem::path(SourceDirectory) / include_file).string();
        if (!std::filesystem::exists(include_file_path))
        {
            LOGEXCEPT("#include is not found in \"{0}\" at line {1}", CurrentFilePath.data(), line_index);
        }
        // did we already include the file
        if (!std::ranges::any_of(pragmaOnce.value(), [&](std::string &t)
                                 { return t == include_file_path; }))
        {
            pragmaOnce->push_back(cpp::UpperCase(include_file_path.string()));
            auto include_source_code = ReadAllText(include_file_path.string());
            if (!include_source_code.has_value())
            {
                LOGEXCEPT("Could not open #include file (READ FAIL) in \"{}\" at line {1}", CurrentFilePath.data(), line_index);
            }
            includesLastModifiedDate.push_back({include_file_path.string(), (uint64_t)std::filesystem::last_write_time(include_file_path.string()).time_since_epoch().count()});
            output += PreprocessIncludeFiles(include_file_path.string(), include_file_path.parent_path().string(), *include_source_code, includesLastModifiedDate, pragmaOnce);
        }
        output.push_back('\n');
        line_index++;
    }
    return output;
}

ShaderReflection Shader::GenerateReflection(const std::vector<uint32_t> &Bytecode, BindingAttributes Attributes)
{
    ShaderReflection output;
    spirv_cross::Compiler compiler(Bytecode);
    const auto &resources = compiler.get_shader_resources();

    auto readStageIO = [&](
                           const spirv_cross::SmallVector<spirv_cross::Resource> &stageIO,
                           std::map<uint32_t, std::map<uint32_t, ShaderReflection::IO>> &out)
    {
        for (auto &item : stageIO)
        {
            ShaderReflection::IO io{};
            const auto &type = compiler.get_type(item.type_id);
            auto baseType = type.basetype;
            io.Location = compiler.get_decoration(item.id, spv::DecorationLocation);
            io.Binding = compiler.get_decoration(item.id, spv::DecorationBinding);
            io.VectorSize = type.vecsize;
            VkFormat format{};
            if (baseType == type.Float)
            {
                io.Size = sizeof(float) * type.vecsize;
                switch (type.vecsize)
                {
                case 1:
                    format = VK_FORMAT_R32_SFLOAT;
                    break;
                case 2:
                    format = VK_FORMAT_R32G32_SFLOAT;
                    break;
                case 3:
                    format = VK_FORMAT_R32G32B32_SFLOAT;
                    break;
                case 4:
                    format = VK_FORMAT_R32G32B32A32_SFLOAT;
                    break;
                }
            }
            else if (baseType == type.Int)
            {
                io.Size = sizeof(int32_t) * type.vecsize;
                switch (type.vecsize)
                {
                case 1:
                    format = VK_FORMAT_R32_SINT;
                    break;
                case 2:
                    format = VK_FORMAT_R32G32_SINT;
                    break;
                case 3:
                    format = VK_FORMAT_R32G32B32_SINT;
                    break;
                case 4:
                    format = VK_FORMAT_R32G32B32A32_SINT;
                    break;
                }
            }
            else if (baseType == type.UInt)
            {
                io.Size = sizeof(uint32_t) * type.vecsize;
                switch (type.vecsize)
                {
                case 1:
                    format = VK_FORMAT_R32_UINT;
                    break;
                case 2:
                    format = VK_FORMAT_R32G32_UINT;
                    break;
                case 3:
                    format = VK_FORMAT_R32G32B32_UINT;
                    break;
                case 4:
                    format = VK_FORMAT_R32G32B32A32_UINT;
                    break;
                }
            }
            else
            {
                LOGEXCEPT("Cannot Generate Reflection Supported Types are Float32, Int32, and UInt32");
            }
            io.Format = format;
            io.Name = compiler.get_name(item.id);
            out[io.Binding][io.Location] = io;
        }
    };
    readStageIO(resources.stage_inputs, output.IOBindingToManyLocationIn);
    readStageIO(resources.stage_outputs, output.IOBindingToManyLocationOut);

    if (resources.push_constant_buffers.size() > 0)
    {
        auto &buffer = resources.push_constant_buffers[0];
        auto &type = compiler.get_type(buffer.type_id);
        output.Pushconstant.Name = compiler.get_name(buffer.id);
        output.Pushconstant.Offset = (uint32_t)compiler.get_decoration(buffer.id, spv::DecorationOffset);
        output.Pushconstant.Size = (uint32_t)compiler.get_declared_struct_size(type);
        output.Pushconstant.HasValue = true;
    }

    auto readBufferResources = [&](
                                   const spirv_cross::SmallVector<spirv_cross::Resource> &buffers,
                                   VkDescriptorType typeNonDynamic, VkDescriptorType typeDynamic)
    {
        for (auto &item : buffers)
        {
            auto &type = compiler.get_type(item.type_id);
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

    auto readImageResources = [&](const spirv_cross::SmallVector<spirv_cross::Resource> &images, VkDescriptorType Type)
    {
        for (auto &item : images)
        {
            auto &type = compiler.get_type(item.type_id);
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
    // readImageResources(resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER);

    return output;
}

std::vector<uint32_t> Shader::CompileGlslToBytecode(const std::string &sourceCode, Type type,
                                                    const PreprocessDefines &defines, bool compileDebug,
                                                    const std::string &fileName)
{
    shaderc_shader_kind shaderKind{};
    switch (type)
    {
    case Type::Vertex:
        shaderKind = shaderc_shader_kind::shaderc_vertex_shader;
        break;
    case Type::Fragment:
        shaderKind = shaderc_shader_kind::shaderc_fragment_shader;
        break;
    case Type::Compute:
        shaderKind = shaderc_shader_kind::shaderc_compute_shader;
        break;
    default:
        LOGEXCEPT("Invalid Shader Type, supported types are vertex, fragment, and compute.");
    }
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    // Supported by Vulkan 1.2
    options.SetTargetSpirv(shaderc_spirv_version_1_5);

    options.SetWarningsAsErrors();
    if (compileDebug)
    {
        options.SetGenerateDebugInfo();
        options.SetOptimizationLevel(shaderc_optimization_level_zero);
    }
    else
    {
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
    }

    std::string defineList;
    for (auto &[preprocessor, value] : defines.Defines)
    {
        defineList += cpp::Format("#define {0} {1}\n", preprocessor, value);
    }
    // defines must be after #version
    // auto versionIndex = cpp::IndexOf(sourceCode, "#version");
    // std::string_view code_without_version = sourceCode.substr(versionIndex);
    // code_without_version = code_without_version.substr(cpp::IndexOf(code_without_version, "\n"));
    // std::string_view version = sourceCode.substr(versionIndex, code_without_version.data() - sourceCode.data());
    // std::stringstream code_stream;
    // code_stream << version << '\n'
    //            << defineList << code_without_version;
    auto code = sourceCode; // code_stream.str();

    auto start = std::chrono::high_resolution_clock::now();
    auto compilationResult = compiler.CompileGlslToSpv(code, shaderKind, fileName.data(), "main", options);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (compilationResult.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success)
    {
        if (fileName.length() > 0)
        {
            LOGEXCEPT("Encountered error during compilation.\nFile:{0}\n{1}", fileName.data(), compilationResult.GetErrorMessage());
        }
        LOGEXCEPT("Encountered error during compilation.\n{1}", compilationResult.GetErrorMessage());
    }
    if (compilationResult.GetNumWarnings() > 0)
    {
        if (fileName.length() > 0)
        {
            LOG(WARNING, "{0} Warnings for Shader \"{1}\"", compilationResult.GetNumWarnings(), fileName.data());
        }
    }
    auto bytecode = std::vector<uint32_t>(compilationResult.begin(), compilationResult.end());
    return bytecode;
}

Shader::DataWrapper::~DataWrapper()
{
    if (m_Ctx && m_Module)
    {
        m_Ctx->Device.destroyShaderModule(m_Module);
    }
}

std::string ShaderReflection::DumpAsText() const
{
    std::string output;

    for (auto &[binding, item_] : IOBindingToManyLocationIn)
    {
        for (auto &[location, item] : item_)
        {
            output +=
                Format("{{Binding={0} Location={1} Format={2} Size={3}}} {4} In;\n",
                       item.Binding, item.Location,
                       vk::to_string(vk::Format(item.Format)), item.Size,
                       item.Name);
        }
    }

    for (auto &[binding, item_] : IOBindingToManyLocationOut)
    {
        for (auto &[location, item] : item_)
        {
            output +=
                Format("{{Binding={0} Location={1} Format={2} Size={3}}} {4} Out;\n",
                       item.Binding, item.Location,
                       vk::to_string(vk::Format(item.Format)), item.Size,
                       item.Name);
        }
    }

    if (Pushconstant.HasValue)
    {
        output += Format("(push_constant) {{Offset={0} Size={1}}} {2}\n", Pushconstant.Offset, Pushconstant.Size, Pushconstant.Name);
    }

    for (auto &[setId, binding] : SetToManyBindings)
    {
        for (auto &[bindingId, item] : binding)
        {
            output += Format("(set={0}, binding={1}) {{Size={2} ArrayCount={3}}} {4}\n", setId, bindingId, item.Size, item.DescriptorCount, item.Name);
        }
    }

    return output;
}

vector<vk::PushConstantRange> Shader::GetPushconstants(const std::initializer_list<Shader> &shaders)
{
    vector<vk::PushConstantRange> ranges;
    for (auto &shader : shaders)
    {
        if (shader.m_Reflection.Pushconstant.HasValue) {
            ranges.push_back(vk::PushConstantRange(
                vk::ShaderStageFlagBits(shader.m_Type),
                shader.m_Reflection.Pushconstant.Offset,
                shader.m_Reflection.Pushconstant.Size));
        }
    }
    return ranges;
}

std::map<uint32_t, vk::DescriptorSetLayout> egx::Shader::CreateDescriptorSetLayouts(const std::initializer_list<Shader> &shaders)
{
    if (shaders.size() == 0)
        return {};
    map<uint32_t, vk::DescriptorSetLayout> setLayouts;
    map<uint32_t, map<uint32_t, vk::DescriptorSetLayoutBinding>> setMap;
    const auto &ctx = shaders.begin()->m_Data->m_Ctx;
    for (auto &shader : shaders)
    {
        const auto &reflection = shader.m_Reflection;
        for (auto &[setId, bindings] : reflection.SetToManyBindings)
        {
            auto& bindingsMap = setMap[setId];
            for (auto &[bindingId, binding] : bindings)
            {
                if (bindingsMap.contains(bindingId))
                {
                    if (bindingsMap[bindingId].descriptorCount != binding.DescriptorCount)
                    {
                        throw runtime_error(cpp::Format("Cannot create set layouts because their is a mismatch on descriptor count on set={} binding={}", setId, bindingId));
                    }
                    if (bindingsMap[bindingId].descriptorType != vk::DescriptorType(binding.Type))
                    {
                        throw runtime_error(cpp::Format("Cannot create set layouts because their is a mismatch on descriptor type on set={} binding={}", setId, bindingId));
                    }
                    bindingsMap[bindingId].stageFlags |= vk::ShaderStageFlagBits(shader.m_Type);
                }
                else
                {
                    bindingsMap[bindingId].binding = bindingId;
                    bindingsMap[bindingId].descriptorCount = binding.DescriptorCount;
                    bindingsMap[bindingId].descriptorType = vk::DescriptorType(binding.Type);
                    bindingsMap[bindingId].stageFlags = vk::ShaderStageFlagBits(shader.m_Type);
                }
            }
        }
    }
    for (auto &[setId, bindings] : setMap)
    {
        vector<vk::DescriptorSetLayoutBinding> setLayoutBindings;
        for (auto &[bindingId, bindingInfo] : bindings)
        {
            setLayoutBindings.push_back(bindingInfo);
        }
        vk::DescriptorSetLayoutCreateInfo createInfo;
        createInfo.setBindings(setLayoutBindings);
        setLayouts[setId] = ctx->Device.createDescriptorSetLayout(createInfo);
    }
    return setLayouts;
}

ShaderReflection egx::ShaderReflection::Combine(const std::initializer_list<ShaderReflection> &reflections)
{
    ShaderReflection result;
    for (auto &reflection : reflections)
    {
        result.ShaderStage |= reflection.ShaderStage;
        for(auto& [setId, bindingInfo] : reflection.SetToManyBindings) {
            if(result.SetToManyBindings.contains(setId)) {
                auto& resultSet = result.SetToManyBindings.at(setId);
                for(auto& [bindingId, binding] : bindingInfo) {
                    if(resultSet.contains(bindingId)) {
                        // Verify both reflections have the same binding.
                        // If not throw exception
                        const auto& resultBinding = resultSet.at(bindingId);
                        if(resultBinding.DescriptorCount != binding.DescriptorCount ||
                            resultBinding.Type != binding.Type) {
                                throw runtime_error(cpp::Format("Cannot combine reflections because mismatch at set={}, binding={}", setId, bindingId));
                        }
                    } else {
                        resultSet[bindingId] = binding;
                    }
                }
            } else {
                result.SetToManyBindings[setId] = bindingInfo;
            }
        }
    }
    return result;
}
