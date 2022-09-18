#include "egxpipelinestate.hpp"

egx::PipelineState::PipelineState(ref<VulkanCoreInterface>& CoreInterface, uint32_t Width, uint32_t Height) : 
    _core(CoreInterface), Width(Width), Height(Height)
{
    this->Layout = egx::PipelineLayout::FactoryCreate(CoreInterface);
}

egx::PipelineState::PipelineState(PipelineState&& move)
{
    memcpy(this, &move, sizeof(PipelineState));
    memset(&move, 0, sizeof(PipelineState));
}

egx::PipelineState& egx::PipelineState::operator=(PipelineState& move)
{
    memcpy(this, &move, sizeof(PipelineState));
    memset(&move, 0, sizeof(PipelineState));
    return *this;
}

void egx::PipelineState::AddSet(ref<egxshaderset>& set)
{
    assert(!Pipeline.IsValidRef());
    Layout->AddSet(set);
}

void egx::PipelineState::AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages)
{
    assert(!Pipeline.IsValidRef());
    Layout->AddPushconstantRange(offset, size, stages);
}

void egx::PipelineState::CreateColorAttachment(uint32_t ColorAttachmentID, VkFormat Format, VkClearValue ClearValue, VkImageUsageFlags CustomUsageFlags, VkImageLayout ImageLayout, VkAttachmentLoadOp LoadOp, VkAttachmentStoreOp StoreOp, VkPipelineColorBlendAttachmentState* pBlendState)
{
    assert(!Pipeline.IsValidRef() && Width > 0 && Height > 0);
    if (!Framebuffer.IsValidRef())
        Framebuffer = egx::egxframebuffer::FactoryCreate(_core, Width, Height);
    Framebuffer->CreateColorAttachment(ColorAttachmentID, Format, ClearValue, CustomUsageFlags, ImageLayout, LoadOp, StoreOp, pBlendState);
}

void egx::PipelineState::CreateDepthAttachment(VkFormat Format, VkClearValue ClearValue, VkImageUsageFlags CustomUsageFlags, VkImageLayout ImageLayout, VkAttachmentLoadOp LoadOp, VkAttachmentStoreOp StoreOp)
{
    assert(!Pipeline.IsValidRef() && Width > 0 && Height > 0);
    if (!Framebuffer.IsValidRef())
        Framebuffer = egx::egxframebuffer::FactoryCreate(_core, Width, Height);
    Framebuffer->CreateDepthAttachment(Format, ClearValue, CustomUsageFlags, ImageLayout, LoadOp, StoreOp);
}

void egx::PipelineState::CreateRasterPipeline(const egxshader& vertex, const egxshader& fragment, const egxvertexdescription& vertexDescription)
{
    assert(!Pipeline.IsValidRef() && Width > 0 && Height > 0);
    this->_isgraphics = true;
    Pipeline = egx::Pipeline::FactoryCreate(_core);
    Pipeline->create(Layout, vertex, fragment, Framebuffer, vertexDescription);
}

void egx::PipelineState::CreateCompuePipeline(const egxshader& compute)
{
    assert(!Pipeline.IsValidRef() && Width == 0 && Height == 0);
    this->_isgraphics = false;
    Pipeline = egx::Pipeline::FactoryCreate(_core);
    Pipeline->create(Layout, compute);
}

void egx::PipelineState::ReloadComputePipeline(const egxshader& compute)
{
    assert(!_isgraphics);
    Pipeline = egx::Pipeline::FactoryCreate(_core);
    Pipeline->create(Layout, compute);
}

void egx::PipelineState::ReloadRasterPipeline(const egxshader& vertex, const egxshader& fragment, const egxvertexdescription& vertexDescription)
{
    assert(_isgraphics);
    Pipeline = egx::Pipeline::FactoryCreate(_core);
    Pipeline->create(Layout, vertex, fragment, Framebuffer, vertexDescription);
}
