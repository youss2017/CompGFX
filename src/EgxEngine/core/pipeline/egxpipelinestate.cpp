#include "egxpipelinestate.hpp"

egx::PipelineState::PipelineState(const ref<VulkanCoreInterface>& CoreInterface, uint32_t Width, uint32_t Height) :
	_core(CoreInterface), Width(Width), Height(Height)
{
	this->Layout = egx::PipelineLayout::FactoryCreate(CoreInterface);
	this->Pipeline = egx::Pipeline::FactoryCreate(CoreInterface);
	if (Width > 0)
		this->Framebuffer = egx::Framebuffer::FactoryCreate(CoreInterface, Width, Height);
}

egx::PipelineState::PipelineState(const ref<VulkanCoreInterface>& CoreInterface, const ref<egx::Framebuffer>& Framebuffer) :
	_core(CoreInterface), Width(Framebuffer->Width), Height(Framebuffer->Height), Framebuffer(Framebuffer)
{
	this->Layout = egx::PipelineLayout::FactoryCreate(CoreInterface);
	this->Pipeline = egx::Pipeline::FactoryCreate(CoreInterface);
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
	Layout->AddSet(set);
}

void egx::PipelineState::AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages)
{
	Layout->AddPushconstantRange(offset, size, stages);
}

void egx::PipelineState::CreateRasterPipeline(const egxshader& vertex, const egxshader& fragment, const uint32_t PassId, const egxvertexdescription& vertexDescription)
{
	assert(Width > 0 && Height > 0);
	Pipeline->invalidate(Layout, vertex, fragment, Framebuffer, PassId, vertexDescription);
}

void egx::PipelineState::CreateCompuePipeline(const egxshader& compute)
{
	assert(Width == 0 && Height == 0);
	Pipeline->invalidate(Layout, compute);
}
