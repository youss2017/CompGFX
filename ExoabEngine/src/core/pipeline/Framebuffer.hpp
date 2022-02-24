#pragma once
#include "../memory/Texture2.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <optional>

struct FramebufferAttachment
{
    TextureFormat m_format;
    VkAttachmentLoadOp m_loading;
    VkAttachmentStoreOp m_storing;
    VkImageLayout initialLayout;
    VkImageLayout finalLayout;
    VkImageLayout imageLayout;
    VkClearValue clearColor;
    VkPipelineColorBlendAttachmentState BlendState;

    // The blend state is only used for color attachments, in this function the blend state is disabled.
    static FramebufferAttachment InitalizeNoBlend(TextureFormat format,
                                                  VkAttachmentLoadOp loading,
                                                  VkAttachmentStoreOp storing,
                                                  VkImageLayout initialLayout,
                                                  VkImageLayout finalLayout,
                                                  VkImageLayout imageLayout,
                                                  VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 0.0f})
    {
        FramebufferAttachment attachment;
        attachment.m_format = format;
        attachment.m_loading = loading;
        attachment.m_storing = storing;
        attachment.initialLayout = initialLayout;
        attachment.finalLayout = finalLayout;
        attachment.imageLayout = imageLayout;
        attachment.clearColor = clearColor;

        attachment.InitalizeBlend();

        return attachment;
    }

    // The default arguments disable blending, the red, green, blue, alpha are flags for the color write mask
    // color write mask allows you to prevent certain color channels from being written into the color attachment
    void InitalizeBlend(
        bool BlendEnable = false,
        VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        VkBlendOp colorBlendOp = VK_BLEND_OP_ADD,
        VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD,
        bool red = true,
        bool green = true,
        bool blue = true,
        bool alpha = true)
    {

        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.colorWriteMask = (red ? VK_COLOR_COMPONENT_R_BIT : 0) | (green ? VK_COLOR_COMPONENT_G_BIT : 0) | (blue ? VK_COLOR_COMPONENT_B_BIT : 0) | (alpha ? VK_COLOR_COMPONENT_A_BIT : 0);
        colorBlendAttachment.blendEnable = BlendEnable;
        colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
        colorBlendAttachment.colorBlendOp = colorBlendOp;
        colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
        colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
        colorBlendAttachment.alphaBlendOp = alphaBlendOp;

        BlendState = colorBlendAttachment;
    }
};

struct FramebufferStateManagement
{
    int m_ApiType = -1;
    float m_BlendConstantRed = 0.f, m_BlendConstantGreen = 0.f, m_BlendConstantBlue = 0.f, m_BlendConstantAlpha = 0.f;
    GraphicsContext m_context = nullptr;
    VkRenderPass m_renderpass = nullptr;
    std::vector<FramebufferAttachment> m_attachments;
    std::optional<FramebufferAttachment> m_depth_attachment;
    uint32_t m_ColorAttachmentCount = 0;
    TextureSamples m_Samples = TextureSamples::MSAA_1;
};

typedef FramebufferStateManagement* IFramebufferStateManagement;

typedef IFramebufferStateManagement PFN_FramebufferStateManagement_Create(GraphicsContext context, std::vector<FramebufferAttachment> attachments, TextureSamples samples,
    float BlendConstantRed, float BlendConstantGreen, float BlendConstantBlue, float BlendConstantAlpha);
typedef void PFN_FramebufferStateManagment_Destroy(IFramebufferStateManagement state_managment);

extern PFN_FramebufferStateManagement_Create* FramebufferStateManagement_Create;
extern PFN_FramebufferStateManagment_Destroy* FramebufferStateManagment_Destroy;

void FramebufferStateManagment_LinkFunctions(GraphicsContext context);

struct Framebuffer
{
    int m_ApiType;
    GraphicsContext m_context;
    uint32_t m_width, m_height;
    std::vector<ITexture2> m_attachments;
    std::vector<VkFramebuffer> m_framebuffers;
    uint32_t m_framebuffer;

    int m_viewport_x, m_viewport_y, m_viewport_width, m_viewport_height;
    int m_scissor_x, m_scissor_y, m_scissor_width, m_scissor_height;

    template <typename T, typename TT, typename TTT, typename TTTT>
    inline void SetViewport(T x, TT y, TTT width, TTTT height)
    {
        m_viewport_x = (int)x, m_viewport_y = (int)y, m_viewport_width = (int)width, m_viewport_height = (int)height;
    }

    template <typename T, typename TT, typename TTT, typename TTTT>
    inline void SetScissor(T x, TT y, TTT width, TTTT height)
    {
        m_scissor_x = (int)x, m_scissor_y = (int)y, m_scissor_width = (int)width, m_scissor_height = (int)height;
    }

    template <typename T, typename TT, typename TTT, typename TTTT>
    inline void GetViewport(T& x, TT& y, TTT& width, TTTT& height) {
        x = m_viewport_x, y = m_viewport_y, width = m_viewport_width, height = m_viewport_height;
    }

    template <typename T, typename TT, typename TTT, typename TTTT>
    inline void GetScissor(T& x, TT& y, TTT& width, TTTT& height) {
        x = m_scissor_x, y = m_scissor_y, width = m_scissor_width, height = m_scissor_height;
    }
};

typedef Framebuffer *IFramebuffer;

typedef IFramebuffer PFN_Framebuffer_Create(GraphicsContext context, uint32_t width, uint32_t height, IFramebufferStateManagement StateManagment);
typedef APIHandle PFN_Framebuffer_Get(IFramebuffer framebuffer);
typedef void PFN_Framebuffer_Destroy(IFramebuffer framebuffer);

extern PFN_Framebuffer_Create* Framebuffer_Create;
extern PFN_Framebuffer_Get* Framebuffer_Get;
extern PFN_Framebuffer_Destroy* Framebuffer_Destroy;

inline VkImage Framebuffer_GetImage(IFramebuffer framebuffer, uint32_t AttachmentIndex, int FrameIndex)
{
    return framebuffer->m_attachments[AttachmentIndex]->m_vk_images_per_frame[FrameIndex];
}

inline VkImageView Framebuffer_GetView(IFramebuffer framebuffer, uint32_t AttachmentIndex, int FrameIndex)
{
    return framebuffer->m_attachments[AttachmentIndex]->m_vk_views_per_frame[FrameIndex];
}

void Framebuffer_LinkFunctions(GraphicsContext context);
