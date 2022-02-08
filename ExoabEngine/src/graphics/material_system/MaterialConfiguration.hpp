#pragma once
#include "../../core/pipeline/Pipeline.hpp"
#include <vector>

struct MaterialBlendStateSettings
{
    bool blend_enable = false;
    VkBlendFactor src_color_blend_factor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dst_color_blend_factor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp color_blend_op = VK_BLEND_OP_ADD;
    VkBlendFactor src_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alpha_blend_op = VK_BLEND_OP_ADD;
    bool ColorWriteMask_r = true;
    bool ColorWriteMask_g = true;
    bool ColorWriteMask_b = true;
    bool ColorWriteMask_a = true;
};

struct MaterialAttachment
{
    uint32_t index;
    uint32_t m_reserve_id; // -1 (0xff...ff) means no reserve id
    uint32_t blend_state_id;
    VkAttachmentLoadOp m_loadop = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp m_storeop = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkClearValue m_clearvalue = {0.0};
    VkImageLayout m_initialLayout;
    VkImageLayout m_imageLayout;
    VkImageLayout m_finialLayout;
    TextureFormat format;
};

#if 0
enum class MaterialBindingElementType
{
    vec2,
    vec4,
    mat4
};
struct MaterialBindingElement
{
    MaterialBindingElementType m_type;
    uint32_t m_size;
    uint32_t m_arraysize;
};
struct MaterialBindingDesc
{
    bool m_istexture = false;
    union
    {
        size_t m_size = 0;
        uint32_t m_texturedimensions;
    };
    uint32_t m_arraysize = 1;
    // Not for textures
    std::vector<MaterialBindingElement> m_elements;
    VkShaderStageFlags m_stages = (VkShaderStageFlags)0;
};
struct MaterialSetDesc
{
    std::vector<MaterialBindingDesc> m_bindings;
};
#endif

class MaterialConfiguration
{
public:
    MaterialConfiguration() {}
    MaterialConfiguration(const char *material_config);

public:
    std::string vertex_shader, fragment_shader;
    uint32_t mc_id;
    std::vector<MaterialAttachment> m_attachments;
#if 0
    std::vector<MaterialSetDesc> m_sets;
#endif
    std::vector<MaterialBlendStateSettings> m_blend_settings;
    uint32_t m_framebuffer_width = -1, m_framebuffer_height = -1;
    /*
        Pipeline Settings
    */
    bool DetectedPipeline = false, InsidePipeline = false;
    CullMode cullmode;
    bool CCWFrontFace;
    PolygonMode polygonmode;
    bool DepthTestEnable;
    bool DepthWriteEnable;
    DepthFunction compareop;
    TextureSamples samples;
    bool SampleShading;
    float minSampleShading;
};
