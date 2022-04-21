#pragma once
#include "Shader.hpp"
#include <vector>
#include <spirv_cross/spirv_cross.hpp>
#include <vulkan/vulkan.h>

// TODO: Support instancing and other vertex bindings
struct VertexAttribute
{
    std::string m_name;
    spirv_cross::SPIRType::BaseType m_type;
    int m_location; // layout (location = 0) in ...
    int m_vecsize;  // vector size (for scalars it is 1)
    int m_size;     // size in bytes
    int m_set;
    int m_offset;
};

struct UniformBufferElement
{
    std::string m_Name;
    uint32_t m_Offset, m_Size;
    uint32_t m_Width;
    uint32_t m_Vecsize;
    uint32_t m_Columns;                         // for matrices
    spirv_cross::SPIRType::BaseType m_basetype; // float, int, vec2, vec3, mat4, etc...
    spirv_cross::SPIRType m_Type;
};

struct UniformBufferDescription
{
    std::string m_Name;
    uint32_t m_Binding, m_Size, m_ArraySize;
    std::vector<UniformBufferElement> m_Elements;
    shaderc_shader_kind m_shaderStage;
};

struct ShaderStorageBufferDescription
{
    std::string m_Name;
    uint32_t m_Binding;
    shaderc_shader_kind m_shaderStage;
};

struct SampledImageDescription
{
    std::string m_Name;
    uint32_t m_Binding, m_Dimensions, m_ArraySize; // 1 = 1D, 2 = 2D, 3 = 3D
    spv::ImageFormat m_Format;
    shaderc_shader_kind m_shaderStage;
};

struct DescriptorSetDescription
{
    uint32_t m_SetID, m_BindingCount = 0;
    std::vector<UniformBufferDescription> m_UniformBuffers;
    std::vector<ShaderStorageBufferDescription> m_SSBOs;
    std::vector<SampledImageDescription> m_SampledImage;
    // TODO:
    // ssbos
    // other images
    // samplers
    // etc
};

class ShaderReflection
{
public:
    ShaderReflection() {}
    ShaderReflection(Shader *shader);

    // These functions are only for vertex shaders
    const uint32_t GetInputLayoutStride();
    const std::vector<uint32_t> GetVertexAttributeLocations();
    VertexAttribute &GetVertexAttribute(int location);
    inline const std::vector<DescriptorSetDescription> &GetShaderSetDescriptions() { return m_ShaderSetDescriptions; }
    inline const spirv_cross::ShaderResources &GetShaderResources() { return m_resources; }
    inline const bool HasPushconstantBuffer() { return m_HasPushconstantBuffer; }
    inline const UniformBufferDescription &GetPushconstantBuffer() { return m_PushconstantBuffer; }

    static ShaderReflection CombineReflections(ShaderReflection* a, ShaderReflection* b);

    UniformBufferDescription* GetBufferDescription(int setID, int bindingID, int* pSetIDMissing);
    SampledImageDescription* GetSampledImageDescription(int setID, int bindingID, int* pSetIDMissing);

private:
    spirv_cross::ShaderResources m_resources;
    std::vector<VertexAttribute> m_VertexAttributes;
    std::vector<DescriptorSetDescription> m_ShaderSetDescriptions;
    UniformBufferDescription m_PushconstantBuffer;
    bool m_HasPushconstantBuffer = false;
    uint32_t m_InputLayoutStride = 0;
    Shader *m_shader = nullptr;
};

std::vector<VkPushConstantRange> GRAPHICS_API ShaderReflection_CombinePushconstantRanges(ShaderReflection* a, ShaderReflection* b);
VkShaderStageFlags GRAPHICS_API ShaderReflection_GetVulkanShaderStage(shaderc_shader_kind shader_kind);
VkFormat GRAPHICS_API ShaderReflection_GetVulkanFormat(spirv_cross::SPIRType::BaseType basetype, uint32_t vecsize);