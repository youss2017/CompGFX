#include "Framebuffer.hpp"
#include "../backend/VkGraphicsCard.hpp"
#include "../memory/vulkan_memory.h"
#include <cassert>
#include <glad/glad.h>

PFN_FramebufferStateManagement_Create *FramebufferStateManagement_Create = nullptr;
PFN_FramebufferStateManagment_Destroy *FramebufferStateManagment_Destroy = nullptr;

IFramebufferStateManagement Vulkan_FramebufferStateManagement_Create(GraphicsContext context, std::vector<FramebufferAttachment> attachments, TextureSamples Samples, float BlendConstantRed, float BlendConstantGreen, float BlendConstantBlue, float BlendConstantAlpha)
{
    auto IsAttachmentColor = [](FramebufferAttachment & attachment) throw()->bool
    {
        if (attachment.m_format == TextureFormat::D32F)
            return false;
        return true;
    };
    VkAttachmentDescription *pAttachments = (VkAttachmentDescription *)alloca(sizeof(VkAttachmentDescription) * attachments.size());
    auto vcont = ToVKContext(context);

    // clamp msaa
    Samples = (TextureSamples)Utils::ClampValues<uint64_t>((uint64_t)Samples, (uint64_t)1, (uint64_t)vcont->m_MaxMSAASamples);

    for (size_t i = 0; i < attachments.size(); i++)
    {
        pAttachments[i].flags = 0;
        pAttachments[i].format = (VkFormat)attachments[i].m_format;
        pAttachments[i].samples = (VkSampleCountFlagBits)Samples;
        pAttachments[i].loadOp = attachments[i].m_loading;
        pAttachments[i].storeOp = attachments[i].m_storing;
        pAttachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachments[i].initialLayout = attachments[i].initialLayout;
        pAttachments[i].finalLayout = attachments[i].finalLayout;
    }

    std::vector<VkAttachmentReference> pColorReferences;
    std::vector<FramebufferAttachment> color_attachments;
    for (size_t i = 0; i < attachments.size(); i++)
    {
        auto &att = attachments[i];
        if (IsAttachmentColor(att))
        {
            VkAttachmentReference reference;
            reference.attachment = i;
            reference.layout = att.imageLayout;
            pColorReferences.push_back(reference);
            color_attachments.push_back(att);
        }
    }

    // get depth reference
    int DepthReferenceCount = 0;
    VkAttachmentReference DepthReference;
    FramebufferAttachment depth_attachment;
    for (size_t i = 0; i < attachments.size(); i++)
    {
        auto &att = attachments[i];
        if (!IsAttachmentColor(att))
        {
            depth_attachment = att;
            DepthReference.attachment = i;
            DepthReference.layout = att.imageLayout;
            DepthReferenceCount++;
        }
    }
    assert(DepthReferenceCount < 2);

    VkSubpassDescription subpass;
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = pColorReferences.size();
    subpass.pColorAttachments = pColorReferences.data();
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = (DepthReferenceCount) ? &DepthReference : nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.attachmentCount = attachments.size();
    createInfo.pAttachments = pAttachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 0;
    createInfo.pDependencies = nullptr;

    VkRenderPass renderpass;
    vkcheck(vkCreateRenderPass(vcont->defaultDevice, &createInfo, vcont->m_allocation_callback, &renderpass));

    FramebufferStateManagement *statem = new FramebufferStateManagement();
    statem->m_ApiType = 0;
    statem->m_context = context;
    statem->m_attachments = color_attachments;
    statem->m_depth_attachment = depth_attachment;
    statem->m_renderpass = renderpass;
    statem->m_ColorAttachmentCount = pColorReferences.size();
    statem->m_BlendConstantRed = BlendConstantRed;
    statem->m_BlendConstantGreen = BlendConstantGreen;
    statem->m_BlendConstantBlue = BlendConstantBlue;
    statem->m_BlendConstantAlpha = BlendConstantAlpha;
    statem->m_Samples = Samples;
    return statem;
}

void Vulkan_FramebufferStateManagment_Destroy(IFramebufferStateManagement state_managment)
{
    auto vcont = ToVKContext(state_managment->m_context);
    vkDestroyRenderPass(vcont->defaultDevice, state_managment->m_renderpass, vcont->m_allocation_callback);
    delete state_managment;
}

IFramebufferStateManagement GL_FramebufferStateManagement_Create(GraphicsContext context, std::vector<FramebufferAttachment> attachments, TextureSamples Samples, float BlendConstantRed, float BlendConstantGreen, float BlendConstantBlue, float BlendConstantAlpha)
{
    auto IsAttachmentColor = [](FramebufferAttachment & attachment) throw()->bool
    {
        if (attachment.m_format == TextureFormat::D32F)
            return false;
        return true;
    };
    std::vector<FramebufferAttachment> color_attachments;
    for (size_t i = 0; i < attachments.size(); i++)
    {
        auto &att = attachments[i];
        if (IsAttachmentColor(att))
        {
            color_attachments.push_back(att);
        }
    }

    // get depth reference
    int DepthReferenceCount = 0;
    FramebufferAttachment depth_attachment;
    for (size_t i = 0; i < attachments.size(); i++)
    {
        auto &att = attachments[i];
        if (!IsAttachmentColor(att))
        {
            depth_attachment = att;
            DepthReferenceCount++;
        }
    }
    assert(DepthReferenceCount < 2);

    FramebufferStateManagement *statem = new FramebufferStateManagement();
    statem->m_ApiType = 1;
    statem->m_context = context;
    statem->m_attachments = color_attachments;
    statem->m_depth_attachment = depth_attachment;
    statem->m_renderpass = nullptr;
    statem->m_ColorAttachmentCount = 0;
    statem->m_BlendConstantRed = BlendConstantRed;
    statem->m_BlendConstantGreen = BlendConstantGreen;
    statem->m_BlendConstantBlue = BlendConstantBlue;
    statem->m_BlendConstantAlpha = BlendConstantAlpha;
    statem->m_Samples = Samples;
    statem->m_ColorAttachmentCount = color_attachments.size();
    return statem;
}

void GL_FramebufferStateManagment_Destroy(IFramebufferStateManagement state_managment)
{
    delete state_managment;
}

void FramebufferStateManagment_LinkFunctions(GraphicsContext context)
{
    char ApiType = *(char *)context;
    if (ApiType == 0)
    {
        FramebufferStateManagement_Create = Vulkan_FramebufferStateManagement_Create;
        FramebufferStateManagment_Destroy = Vulkan_FramebufferStateManagment_Destroy;
    }
    else if (ApiType == 1)
    {
        FramebufferStateManagement_Create = GL_FramebufferStateManagement_Create;
        FramebufferStateManagment_Destroy = GL_FramebufferStateManagment_Destroy;
    }
    else
    {
        assert(0);
        Utils::Break();
    }
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Framebuffer ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

IFramebuffer Vulkan_Framebuffer_Create(GraphicsContext context, uint32_t width, uint32_t height, IFramebufferStateManagement StateManagment)
{
    auto vcont = ToVKContext(context);
    auto attachment_descriptions = StateManagment->m_attachments;
    int FrameCount = vcont->FrameInfo->m_FrameCount;
    // std::vector<VkImage> images;
    // std::vector<VkImageView> views;

    auto IsAttachmentColor = [](FramebufferAttachment & attachment) throw()->bool
    {
        if (attachment.m_format == TextureFormat::D32F)
            return false;
        return true;
    };

    auto samples = StateManagment->m_Samples;
    std::vector<IGPUTexture2D> attachments;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;

    for (auto &attac : attachment_descriptions)
    {
        bool IsColor = IsAttachmentColor(attac);
        Texture2DSpecification attachment_desc;
        attachment_desc.m_Width = width;
        attachment_desc.m_Height = height;
        attachment_desc.m_TextureUsage = IsColor ? TextureUsage::COLOR_ATTACHMENT : TextureUsage::DEPTH_ATTACHMENT;
        attachment_desc.m_Samples = samples;
        attachment_desc.m_Format = attac.m_format;
        attachment_desc.m_GenerateMipMapLevels = false;
        attachment_desc.m_CreatePerFrame = true;
        attachment_desc.m_LazilyAllocate = true;

        IGPUTexture2D attachment = GPUTexture2D_Create(context, &attachment_desc);
        attachments.push_back(attachment);
        auto ___images = attachment->m_images;
        auto ___views = attachment->m_views;
        images = Utils::CombineVectors(images, ___images);
        views = Utils::CombineVectors(views, ___views);
    }

    std::vector<VkFramebuffer> framebuffers(FrameCount);
    for (int i = 0; i < FrameCount; i++)
    {
        int AttachmentCount = (views.size() / FrameCount);
        VkImageView *attachment_views = (VkImageView *)alloca(sizeof(VkImageView) * AttachmentCount);

        for (int j = 0; j < AttachmentCount; j++)
        {
            attachment_views[j] = views[j * FrameCount + i];
        }

        VkFramebufferCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.renderPass = (VkRenderPass)StateManagment->m_renderpass;
        createInfo.attachmentCount = AttachmentCount;
        createInfo.pAttachments = attachment_views;
        createInfo.width = width;
        createInfo.height = height;
        createInfo.layers = 1;
        VkFramebuffer framebuffer;
        vkcheck(vkCreateFramebuffer(vcont->defaultDevice, &createInfo, vcont->m_allocation_callback, &framebuffer));
        framebuffers[i] = framebuffer;
    }

    Framebuffer *fbo = new Framebuffer();
    fbo->m_ApiType = 0;
    fbo->m_context = vcont;
    fbo->m_width = width;
    fbo->m_height = height;
    fbo->m_framebuffers = framebuffers;
    fbo->m_attachments = attachments;
    return fbo;
}

APIHandle Vulkan_Framebuffer_Get(IFramebuffer framebuffer)
{
    auto vcont = ToVKContext(framebuffer->m_context);
    int frameindex = vcont->FrameInfo->m_FrameIndex;
    return (APIHandle)framebuffer->m_framebuffers[frameindex];
}

void Vulkan_Framebuffer_Destroy(IFramebuffer framebuffer)
{
    auto vcont = ToVKContext(framebuffer->m_context);
    // Destroy Attachments
    for (auto &attachment : framebuffer->m_attachments)
        GPUTexture2D_Destroy(attachment);
    // Destroy Framebuffers
    for (auto &framebuffer : framebuffer->m_framebuffers)
    {
        vkDestroyFramebuffer(vcont->defaultDevice, framebuffer, vcont->m_allocation_callback);
    }
    delete framebuffer;
}

// ======================================== OPENGL ========================================

IFramebuffer GL_Framebuffer_Create(GraphicsContext context, uint32_t width, uint32_t height, IFramebufferStateManagement StateManagment)
{
    GLuint framebufferID;
    glGenFramebuffers(1, &framebufferID);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferID);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    auto attachment_descriptions = StateManagment->m_attachments;

    auto IsAttachmentColor = [](FramebufferAttachment & attachment) throw()->bool
    {
        if (attachment.m_format == TextureFormat::D32F)
            return false;
        return true;
    };

    auto samples = StateManagment->m_Samples;
    std::vector<IGPUTexture2D> attachments;
    int index = 0;
    for (auto &attac : attachment_descriptions)
    {
        bool IsColor = IsAttachmentColor(attac);
        Texture2DSpecification attachment_desc;
        attachment_desc.m_Width = width;
        attachment_desc.m_Height = height;
        attachment_desc.m_TextureUsage = IsColor ? TextureUsage::COLOR_ATTACHMENT : TextureUsage::DEPTH_ATTACHMENT;
        attachment_desc.m_Samples = samples;
        attachment_desc.m_Format = attac.m_format;
        attachment_desc.m_GenerateMipMapLevels = false;
        attachment_desc.m_CreatePerFrame = true;
        attachment_desc.m_LazilyAllocate = true;

        IGPUTexture2D attachment = GPUTexture2D_Create(context, &attachment_desc);
        attachments.push_back(attachment);

        if (samples == TextureSamples::MSAA_1)
        {
            if (IsColor)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, PVOIDToUInt32(attachment->m_NativeHandle), 0);
            else
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, PVOIDToUInt32(attachment->m_NativeHandle), 0);
        }
        else
        {
            assert(0); // am i doing this right?
            if (IsColor)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D_MULTISAMPLE, PVOIDToUInt32(attachment->m_NativeHandle), 0);
            else
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, PVOIDToUInt32(attachment->m_NativeHandle), 0);
        }
        index++;
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        logerror("OpenGL Framebuffer is not Complete!");
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    Framebuffer *fbo = new Framebuffer();
    fbo->m_ApiType = 1;
    fbo->m_attachments = attachments;
    fbo->m_context = context;
    fbo->m_width = width;
    fbo->m_height = height;
    fbo->m_framebuffer = framebufferID;
    fbo->m_viewport_x = fbo->m_scissor_x = 0;
    fbo->m_viewport_y = fbo->m_scissor_y = 0;
    fbo->m_viewport_width = fbo->m_scissor_width = width;
    fbo->m_viewport_height = fbo->m_scissor_height = height;
    return fbo;
}

APIHandle GL_Framebuffer_Get(IFramebuffer framebuffer)
{
    return UInt32ToPVOID(framebuffer->m_framebuffer);
}

void GL_Framebuffer_Destroy(IFramebuffer framebuffer)
{
    for (auto &attachment : framebuffer->m_attachments)
        GPUTexture2D_Destroy(attachment);
    glDeleteFramebuffers(1, &framebuffer->m_framebuffer);
    delete framebuffer;
}

PFN_Framebuffer_Create *Framebuffer_Create = nullptr;
PFN_Framebuffer_Get *Framebuffer_Get = nullptr;
PFN_Framebuffer_Destroy *Framebuffer_Destroy = nullptr;

void Framebuffer_LinkFunctions(GraphicsContext context)
{
    char ApiType = *(char *)context;
    if (ApiType == 0)
    {
        Framebuffer_Create = Vulkan_Framebuffer_Create;
        Framebuffer_Get = Vulkan_Framebuffer_Get;
        Framebuffer_Destroy = Vulkan_Framebuffer_Destroy;
    }
    else if (ApiType == 1)
    {
        Framebuffer_Create = GL_Framebuffer_Create;
        Framebuffer_Get = GL_Framebuffer_Get;
        Framebuffer_Destroy = GL_Framebuffer_Destroy;
    }
    else
    {
        Utils::Break();
    }
}
