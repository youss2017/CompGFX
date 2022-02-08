#include "ShaderReflection.hpp"
#include "../backend/VkGraphicsCard.hpp"
#include "../backend/GLGraphicsCard.hpp"
#include "../../utils/common.hpp"
#include "../../utils/StringUtils.hpp"
#include "../../utils/Logger.hpp"
#include <algorithm>
#include <spirv_cross/spirv_glsl.hpp>

/* Reflection Info: https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide */
constexpr bool PrintReflectionInfo = false;

ShaderReflection::ShaderReflection(Shader *shader)
{
    m_shader = shader;
    spirv_cross::ShaderResources resources;
    spirv_cross::Compiler *compiler = nullptr;
    spirv_cross::Compiler setID_compiler = spirv_cross::Compiler(shader->GetBytecode(), shader->GetWordCount());
    if (shader->m_ApiType == 0)
    {
        compiler = new spirv_cross::Compiler(shader->GetBytecode(), shader->GetWordCount());
        resources = compiler->get_shader_resources();
    }
    else
    {
        compiler = new spirv_cross::CompilerGLSL(shader->GetBytecode(), shader->GetWordCount());
        resources = compiler->get_shader_resources();
    }
#if 0
    auto active_resources = compiler->get_active_interface_variables();
    spirv_cross::ShaderResources resources = compiler->get_shader_resources(active_resources);
    compiler->set_enabled_interface_variables(std::move(active_resources));
#endif
    m_resources = resources;
    //memcpy(&m_resources, &resources, sizeof(spirv_cross::ShaderResources));

    if (shader->GetShaderKind() == shaderc_vertex_shader)
    {
        for (size_t i = 0; i < resources.stage_inputs.size(); i++)
        {
            auto input = resources.stage_inputs[i];
            auto type = compiler->get_type(input.base_type_id);
            VertexAttribute attribute;
            attribute.m_name = input.name;
            attribute.m_type = type.basetype;
            attribute.m_location = compiler->get_decoration(input.id, spv::DecorationLocation);
            attribute.m_vecsize = type.vecsize;
            attribute.m_size = (type.width / 8) * type.vecsize;
            attribute.m_set = setID_compiler.get_decoration(input.id, spv::DecorationDescriptorSet);
            attribute.m_offset = 0;
            m_InputLayoutStride += attribute.m_size;
            m_VertexAttributes.push_back(attribute);
            // printf("%s: [%d, %d]; --- [%s (%d)]\n", input.name.c_str(), attribute.m_set, attribute.m_location, (type.basetype == type.Float) ? "FLOAT" : "OTHER", type.vecsize);
        }
        // calcluate offsets
        for (size_t i = 0; i < m_VertexAttributes.size(); i++)
        {
            auto &attrib = GetVertexAttribute(i);
            if (i > 0)
            {
                auto &last_attrib = GetVertexAttribute(i - 1);
                attrib.m_offset += last_attrib.m_offset + last_attrib.m_size;
            }
            else
            {
                attrib.m_offset = 0;
            }
        }
    }

    auto GetSetPointer = [&](uint32_t setID) throw()->DescriptorSetDescription *
    {
        bool set_exists = false;
        DescriptorSetDescription *pSetDesc = nullptr;
        for (auto &set_desc : m_ShaderSetDescriptions)
        {
            if (set_desc.m_SetID == setID)
            {
                set_exists = true;
                pSetDesc = &set_desc;
                break;
            }
        }
        if (!set_exists)
        {
            DescriptorSetDescription set_desc;
            set_desc.m_SetID = setID;
            m_ShaderSetDescriptions.push_back(set_desc);
            pSetDesc = &m_ShaderSetDescriptions[m_ShaderSetDescriptions.size() - 1];
        }
        return pSetDesc;
    };

    // Uniform Buffer
    for (size_t i = 0; i < resources.uniform_buffers.size(); i++)
    {
        auto &uniform_buffer = resources.uniform_buffers[i];
        auto type = compiler->get_type(uniform_buffer.type_id);
        uint32_t size = compiler->get_declared_struct_size(type);
        uint32_t member_count = type.member_types.size();
        uint32_t set = setID_compiler.get_decoration(uniform_buffer.id, spv::DecorationDescriptorSet);
        uint32_t binding = setID_compiler.get_decoration(uniform_buffer.id, spv::DecorationBinding);
        if (PrintReflectionInfo)
        {
            char buffer[150];
            sprintf(buffer, "%s [%d bytes]; member count %d; set: %d, binding: %d; \n", uniform_buffer.name.c_str(), size, member_count, set, binding);
            lograw(buffer);
        }
        std::string name0 = compiler->get_name(uniform_buffer.id);
        std::string name1 = compiler->get_fallback_name(uniform_buffer.id);
        DescriptorSetDescription *pSetDesc = GetSetPointer(set);
        UniformBufferDescription bufdesc;
        bufdesc.m_Binding = binding;
        if (name0.size() > 0)
        {
            bufdesc.m_Name = name0;
        }
        else
        {
            bufdesc.m_Name = name1;
        }
        bufdesc.m_Size = size;
        bufdesc.m_shaderStage = shader->GetShaderKind();
        for (uint32_t j = 0; j < member_count; j++)
        {
            auto &member_type = compiler->get_type(type.member_types[j]);
            uint32_t member_size = compiler->get_declared_struct_member_size(type, j);
            uint32_t member_offset = compiler->type_struct_member_offset(type, j);
            const std::string &member_name = compiler->get_member_name(type.self, j);
            if (PrintReflectionInfo)
            {
                char buffer[150];
                sprintf(buffer, "\t%s [%d bytes], offset: [%d bytes]; vecsize: %d\n", member_name.c_str(), member_size, member_offset, member_type.vecsize);
                lograw(buffer);
            }
            UniformBufferElement element;
            element.m_Name = member_name;
            element.m_Offset = member_offset;
            element.m_Size = member_size;
            element.m_Width = member_type.width;
            element.m_Vecsize = member_type.vecsize;
            element.m_Columns = member_type.columns;
            element.m_basetype = member_type.basetype;
            element.m_Type = member_type;
            bufdesc.m_Elements.push_back(element);
            pSetDesc->m_BindingCount++;
        }
        // This should always be true
        assert(type.array.size() <= 1);
        bufdesc.m_ArraySize = type.array.size() == 1 ? type.array[0] : 1;
        pSetDesc->m_UniformBuffers.push_back(bufdesc);
        // pSetDesc->m_BindingsTypeList.push_back(DescriptorSetBindingType::UNIFORM_BUFFER);
    }

    // Push Constant
    if (resources.push_constant_buffers.size() > 0)
    {
        m_HasPushconstantBuffer = true;
        auto &pushconstant_buffer = resources.push_constant_buffers[0];
        auto type = compiler->get_type(pushconstant_buffer.base_type_id);
        uint32_t size = compiler->get_declared_struct_size(type);
        uint32_t member_count = type.member_types.size();
        if (PrintReflectionInfo)
        {
            char buffer[150];
            sprintf(buffer, "Pushblock [%s] Size %d; Member Count %d;\n", pushconstant_buffer.name.c_str(), size, member_count);
            lograw(buffer);
        }
        m_PushconstantBuffer.m_Name = pushconstant_buffer.name;
        // for certain cases, pushconstant can actually have a binding, if emit_pushconstant_as_uniform_buffers is enabled
        m_PushconstantBuffer.m_Binding = compiler->get_decoration(pushconstant_buffer.id, spv::DecorationBinding);
        m_PushconstantBuffer.m_Size = size;
        m_PushconstantBuffer.m_shaderStage = shader->GetShaderKind();
        for (uint32_t j = 0; j < member_count; j++)
        {
            auto &member_type = compiler->get_type(type.member_types[j]);
            uint32_t member_size = compiler->get_declared_struct_member_size(type, j);
            uint32_t member_offset = compiler->type_struct_member_offset(type, j);
            const std::string &member_name = compiler->get_member_name(type.self, j);
            if (PrintReflectionInfo)
            {
                char buffer[150];
                sprintf(buffer, "\t%s [%d bytes], offset: [%d bytes]; vecsize: %d\n", member_name.c_str(), member_size, member_offset, member_type.vecsize);
                lograw(buffer);
            }
            UniformBufferElement element;
            element.m_Name = member_name;
            element.m_Offset = member_offset;
            element.m_Size = member_size;
            element.m_Width = type.width;
            element.m_Vecsize = type.vecsize;
            element.m_Columns = type.columns;
            element.m_basetype = member_type.basetype;
            element.m_Type = member_type;
            m_PushconstantBuffer.m_Elements.push_back(element);
        }
        m_PushconstantBuffer.m_Size -= m_PushconstantBuffer.m_Elements[0].m_Offset;
    }

    // Sampled Images
    for (size_t i = 0; i < resources.sampled_images.size(); i++)
    {
        auto &image = resources.sampled_images[i];
        auto type = compiler->get_type(image.type_id);
        uint32_t set = setID_compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding = setID_compiler.get_decoration(image.id, spv::DecorationBinding);
        SampledImageDescription desc;
        desc.m_Name = image.name;
        desc.m_Binding = binding;
        desc.m_Dimensions = type.image.dim + 1;
        desc.m_Format = type.image.format;
        desc.m_shaderStage = shader->GetShaderKind();
        assert(type.array.size() <= 1);
        desc.m_ArraySize = type.array.size() == 1 ? type.array[0] : 1;
        DescriptorSetDescription *pSetDesc = GetSetPointer(set);
        // pSetDesc->m_BindingsTypeList.push_back(DescriptorSetBindingType::SAMPLED_IMAGE);
        pSetDesc->m_SampledImage.push_back(desc);
        pSetDesc->m_BindingCount++;
        if (PrintReflectionInfo)
        {
            char buffer[150];
            sprintf(buffer, "%s; Set (%d), Binding (%d);\n", image.name.c_str(), set, binding);
            lograw(buffer);
        }
    }

    /* These are not supported (for now) so if a shader uses any of the following resources I will be reminded to supported it. */
    // assert(resources.uniform_buffers.size() == 0);
    assert(resources.storage_buffers.size() == 0);
    // assert(resources.stage_inputs.size() == 0);
    // assert(resources.stage_outputs.size() == 0);
    assert(resources.subpass_inputs.size() == 0);
    assert(resources.storage_images.size() == 0);
    // assert(resources.sampled_images.size() == 0);
    assert(resources.atomic_counters.size() == 0);
    assert(resources.acceleration_structures.size() == 0);
    assert(resources.separate_images.size() == 0);
    assert(resources.separate_samplers.size() == 0);

    delete compiler;

    for (auto &setDescription : m_ShaderSetDescriptions)
    {
        auto compareByBinding = [](const UniformBufferDescription &a, const UniformBufferDescription &b) throw()->bool
        {
            return a.m_Binding < b.m_Binding;
        };
        std::sort(setDescription.m_UniformBuffers.begin(), setDescription.m_UniformBuffers.end(), compareByBinding);
    }
}

const uint32_t ShaderReflection::GetInputLayoutStride()
{
    assert(m_shader->GetShaderKind() == shaderc_vertex_shader);
    return m_InputLayoutStride;
}

const std::vector<uint32_t> ShaderReflection::GetVertexAttributeLocations()
{
    assert(m_shader->GetShaderKind() == shaderc_vertex_shader);
    std::vector<uint32_t> locations;
    for (size_t i = 0; i < m_VertexAttributes.size(); i++)
    {
        auto &attrib = m_VertexAttributes[i];
        locations.push_back(attrib.m_location);
    }
    std::sort(locations.begin(), locations.end());
    return locations;
}

VertexAttribute &ShaderReflection::GetVertexAttribute(int location)
{
    assert(m_shader->GetShaderKind() == shaderc_vertex_shader);
    assert(location >= 0 && (size_t)location < m_VertexAttributes.size());
    for (size_t i = 0; i < m_VertexAttributes.size(); i++)
        if (m_VertexAttributes[i].m_location == location)
            return m_VertexAttributes[i];
    Utils::Break();
    return m_VertexAttributes[0];
}

ShaderReflection ShaderReflection::CombineReflections(ShaderReflection *a, ShaderReflection *b)
{
    assert(a && b);
    ShaderReflection combined;

    if (a->m_VertexAttributes.size() > 0 && b->m_VertexAttributes.size() > 0)
    {
        combined.m_VertexAttributes = a->m_VertexAttributes;
        logerror_break("Only one shader reflection can have vertex attributes, otherwise they cannot be combined.");
    }
    else
    {
        if (a->m_VertexAttributes.size() > 0)
            combined.m_VertexAttributes = a->m_VertexAttributes;
        else
            combined.m_VertexAttributes = b->m_VertexAttributes;
    }

    if (a->m_HasPushconstantBuffer && b->m_HasPushconstantBuffer)
    {
        if (!Utils::EqualNotCaseSensitive(a->m_PushconstantBuffer.m_Name, b->m_PushconstantBuffer.m_Name))
        {
            logerror_break("When combining both pushconstant they must have the same name!");
        }
        else
        {
            auto ElementAlreadyExists = [](std::vector<UniformBufferElement>& group, const UniformBufferElement& element) throw()->bool
            {
                for (auto& group_element : group)
                {
                    if (Utils::EqualNotCaseSensitive(group_element.m_Name, element.m_Name))
                    {
                        assert(group_element.m_Size == element.m_Size);
                        return true;
                    }
                }
                return false;
            };
            std::vector<UniformBufferElement> combined_elements = a->m_PushconstantBuffer.m_Elements;
            for (auto& e : b->m_PushconstantBuffer.m_Elements)
            {
                if (!ElementAlreadyExists(b->m_PushconstantBuffer.m_Elements, e))
                    combined_elements.push_back(e);
            }
            size_t size = 0;
            for (auto& e : combined_elements)
                size += e.m_Size;
            UniformBufferDescription pushconstant_description{};
            pushconstant_description.m_ArraySize = a->m_PushconstantBuffer.m_ArraySize;
            pushconstant_description.m_Binding = a->m_PushconstantBuffer.m_Binding;
            pushconstant_description.m_Elements = combined_elements;
            pushconstant_description.m_Name = a->m_PushconstantBuffer.m_Name;
            pushconstant_description.m_shaderStage = (shaderc_shader_kind)(a->m_PushconstantBuffer.m_shaderStage | b->m_PushconstantBuffer.m_shaderStage);
            pushconstant_description.m_Size = size;
            combined.m_PushconstantBuffer = pushconstant_description;
        }
    }
    else {
        if (a->m_HasPushconstantBuffer) {
            combined.m_HasPushconstantBuffer = true;
            combined.m_PushconstantBuffer = a->m_PushconstantBuffer;
        } else if (b->m_HasPushconstantBuffer) {
            combined.m_HasPushconstantBuffer = true;
            combined.m_PushconstantBuffer = b->m_PushconstantBuffer;
        }
    }

    std::vector<DescriptorSetDescription> CombinedShaderSetDescriptions = a->m_ShaderSetDescriptions;

    auto DescriptorSetAlreadyExists = [](std::vector<DescriptorSetDescription> & group, const DescriptorSetDescription &set, int *pExistanceIndex) throw()->bool
    {
        *pExistanceIndex = 0;
        for (auto &group_element : group)
        {
            if (group_element.m_SetID == set.m_SetID)
            {
                return true;
            }
            *pExistanceIndex++;
        }
        return false;
    };

    auto UniformBufferAlreadyExists = [](std::vector<UniformBufferDescription> & group, const UniformBufferDescription &buffer) throw()->bool
    {
        for (auto &group_element : group)
        {
            if (group_element.m_Binding == buffer.m_Binding)
            {
                assert(Utils::EqualNotCaseSensitive(group_element.m_Name, buffer.m_Name));
                assert(group_element.m_Size == buffer.m_Size);
                group_element.m_shaderStage = (shaderc_shader_kind)(group_element.m_shaderStage | buffer.m_shaderStage);
                return true;
            }
        }
        return false;
    };

    auto SampledImageAlreadyExists = [](std::vector<SampledImageDescription> & group, const SampledImageDescription &image) throw()->bool
    {
        for (auto &group_element : group)
        {
            if (group_element.m_Binding == image.m_Binding)
            {
                assert(Utils::EqualNotCaseSensitive(group_element.m_Name, image.m_Name));
                assert(group_element.m_Dimensions == image.m_Dimensions);
                group_element.m_shaderStage = (shaderc_shader_kind)(group_element.m_shaderStage | image.m_shaderStage);
                return true;
            }
        }
        return false;
    };

    for (auto &set : b->m_ShaderSetDescriptions)
    {
        int index;
        if (!DescriptorSetAlreadyExists(CombinedShaderSetDescriptions, set, &index))
        {
            CombinedShaderSetDescriptions.push_back(set);
            continue;
        }
        // Do we have the same bindings?
        auto &otherSet = b->m_ShaderSetDescriptions[index];
        // 1) Combine Buffer Descriptions
        for (auto &buffer : set.m_UniformBuffers)
        {
            if (!UniformBufferAlreadyExists(otherSet.m_UniformBuffers, buffer))
            {
                otherSet.m_UniformBuffers.push_back(buffer);
                continue;
            }
        }
        // 2) Combine Sampled Texture Descriptions
        for (auto &image : set.m_SampledImage)
        {
            if (!SampledImageAlreadyExists(otherSet.m_SampledImage, image))
            {
                otherSet.m_SampledImage.push_back(image);
                continue;
            }
        }
    }

    // Update Descriptor Set Information
    for(auto& set : CombinedShaderSetDescriptions)
    {
        set.m_BindingCount = set.m_UniformBuffers.size() + set.m_SampledImage.size();
    }

    combined.m_ShaderSetDescriptions = CombinedShaderSetDescriptions;

    return combined;
}

UniformBufferDescription *ShaderReflection::GetBufferDescription(int setID, int bindingID, int *pSetIDMissing)
{
    *pSetIDMissing = 1;
    int index = 0;
    for (auto &set_description : m_ShaderSetDescriptions)
    {
        if (set_description.m_SetID == setID)
        {
            *pSetIDMissing = 0;
            int jindex = 0;
            for (auto &buffer_description : set_description.m_UniformBuffers)
            {
                if (buffer_description.m_Binding == bindingID)
                {
                    return &m_ShaderSetDescriptions[index].m_UniformBuffers[jindex];
                }
                jindex++;
            }
            break;
        }
        index++;
    }
    return nullptr;
}

SampledImageDescription *ShaderReflection::GetSampledImageDescription(int setID, int bindingID, int *pSetIDMissing)
{
    *pSetIDMissing = 1;
    int index = 0;
    for (auto &set_description : m_ShaderSetDescriptions)
    {
        if (set_description.m_SetID == setID)
        {
            *pSetIDMissing = 0;
        }
        int jindex = 0;
        for (auto &texture_description : set_description.m_SampledImage)
        {
            if (texture_description.m_Binding == bindingID)
            {
                return &m_ShaderSetDescriptions[index].m_SampledImage[jindex];
            }
            jindex++;
        }
        index++;
    }

    return nullptr;
}

std::vector<VkPushConstantRange> ShaderReflection_CombinePushconstantRanges(ShaderReflection *a, ShaderReflection *b)
{
    assert(a && b);
    std::vector<VkPushConstantRange> ranges;

    if (a->HasPushconstantBuffer())
    {
        const auto &apushconstant = a->GetPushconstantBuffer();
        VkPushConstantRange range;
        range.stageFlags = ShaderReflection_GetVulkanShaderStage(apushconstant.m_shaderStage);
        range.offset = apushconstant.m_Elements[0].m_Offset;
        range.size = apushconstant.m_Size;
        ranges.push_back(range);
    }

    if (b->HasPushconstantBuffer())
    {
        const auto &bpushconstant = b->GetPushconstantBuffer();
        VkPushConstantRange range;
        range.stageFlags = ShaderReflection_GetVulkanShaderStage(bpushconstant.m_shaderStage);
        range.offset = bpushconstant.m_Elements[0].m_Offset;
        range.size = bpushconstant.m_Size;
        ranges.push_back(range);
    }

    return ranges;
}

VkShaderStageFlags ShaderReflection_GetVulkanShaderStage(shaderc_shader_kind shader_kind)
{
    if (shader_kind == shaderc_vertex_shader)
        return VK_SHADER_STAGE_VERTEX_BIT;
    if (shader_kind == shaderc_fragment_shader)
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    if (shader_kind == shaderc_compute_shader)
        return VK_SHADER_STAGE_COMPUTE_BIT;
    if (shader_kind == shaderc_geometry_shader)
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    assert(0);
    return VK_SHADER_STAGE_ALL_GRAPHICS;
}

VkFormat ShaderReflection_GetVulkanFormat(spirv_cross::SPIRType::BaseType basetype, uint32_t vecsize)
{
    VkFormat attribute_format = VK_FORMAT_UNDEFINED;
    // GET FORMAT
    switch (basetype)
    {
    case spirv_cross::SPIRType::BaseType::SByte:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R8_SINT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R8G8_SINT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R8G8B8_SINT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R8G8B8A8_SINT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::UByte:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R8_UINT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R8G8_UINT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R8G8B8_UINT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R8G8B8A8_UINT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::Short:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R16_SINT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16_SINT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16B16_SINT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16B16A16_SINT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::UShort:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R16_UINT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16_UINT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16B16_UINT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16B16A16_UINT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::Int:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R32_SINT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32_SINT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32B32_SINT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32B32A32_SINT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::UInt:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R32_UINT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32_UINT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32B32_UINT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32B32A32_UINT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::Int64:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R64_SINT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64_SINT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64B64_SINT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64B64A64_SINT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::UInt64:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R64_UINT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64_UINT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64B64_UINT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64B64A64_UINT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::Half:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R16_SFLOAT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16_SFLOAT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16B16_SFLOAT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::Float:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R32_SFLOAT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    case spirv_cross::SPIRType::BaseType::Double:
    {
        if (vecsize == 1)
        {
            attribute_format = VkFormat::VK_FORMAT_R64_SFLOAT;
        }
        else if (vecsize == 2)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64_SFLOAT;
        }
        else if (vecsize == 3)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64B64_SFLOAT;
        }
        else if (vecsize == 4)
        {
            attribute_format = VkFormat::VK_FORMAT_R64G64B64A64_SFLOAT;
        }
        else
        {
            assert(0);
        }
    }
    break;
    default:
        assert(0);
    }
    return attribute_format;
}
