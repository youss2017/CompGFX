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
	assert(!Pipeline.IsValidRef());
	Layout->AddSet(set);
}

void egx::PipelineState::AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages)
{
	assert(!Pipeline.IsValidRef());
	Layout->AddPushconstantRange(offset, size, stages);
}

void egx::PipelineState::CreateRasterPipeline(const egxshader& vertex, const egxshader& fragment, const uint32_t PassId, const egxvertexdescription& vertexDescription)
{
	assert(Pipeline->Pipe == nullptr && Width > 0 && Height > 0);
	this->_isgraphics = true;
	Pipeline->create(Layout, vertex, fragment, Framebuffer, PassId, vertexDescription);
}

void egx::PipelineState::CreateCompuePipeline(const egxshader& compute)
{
	assert(Pipeline->Pipe == nullptr && Width == 0 && Height == 0);
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

void egx::PipelineState::ReloadRasterPipeline(const egxshader& vertex, const egxshader& fragment, const uint32_t PassId, const egxvertexdescription& vertexDescription)
{
	assert(_isgraphics);
	Pipeline = egx::Pipeline::FactoryCreate(_core);
	Pipeline->create(Layout, vertex, fragment, Framebuffer, PassId, vertexDescription);
}
