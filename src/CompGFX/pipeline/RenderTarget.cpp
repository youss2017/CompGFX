#include "RenderTarget.hpp"
#include <memory/formatsize.hpp>

using namespace std;
using namespace egx;
using namespace vk;

egx::RenderTarget::RenderTarget(const DeviceCtx& ctx, uint32_t width, uint32_t height)
{
	m_Data = make_shared<RenderTarget::DataWrapper>();
	m_Data->m_Ctx = ctx, m_Data->m_Width = width, m_Data->m_Height = height;
}

egx::RenderTarget::RenderTarget(const DeviceCtx& ctx, const SwapchainController& swapchain, vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout, vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, vk::ClearValue clearColor, vk::AttachmentLoadOp stencilLoadOp, vk::AttachmentStoreOp stencilStoreOp)
	: RenderTarget(ctx, swapchain.Width(), swapchain.Height())
{
	m_SwapchainFlag = true;
	auto backBuffers = swapchain.GetBackBuffers();
	int32_t i = -1;
	for (vk::Image handle : backBuffers)
	{
		auto image = Image2D::CreateFromHandle(ctx, handle, swapchain.Width(), swapchain.Height(), swapchain.GetFormat(), 1, vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eUndefined);
		AddColorAttachment(i, image, image.Format, initialLayout, subpassLayout, finalLayout, loadOp, storeOp, clearColor,
			stencilLoadOp, stencilStoreOp);
		i--;
	}
}

RenderTarget& egx::RenderTarget::CreateColorAttachment(int32_t id, vk::Format format,
	vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
	vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
	vk::ClearValue clearColor, vk::AttachmentLoadOp stencilLoadOp,
	vk::AttachmentStoreOp stencilStoreOp)
{
	if (m_Data->m_ColorAttachments.contains(id))
	{
		throw runtime_error(cpp::Format("Color Attachment already exists with id {}", id));
	}
	Image2D image(m_Data->m_Ctx, m_Data->m_Width, m_Data->m_Height, format, 1,
		ImageUsageFlagBits::eColorAttachment | ImageUsageFlagBits::eSampled | ImageUsageFlagBits::eStorage,
		finalLayout, false);
	vk::AttachmentDescription2 attachment;
	attachment.setFormat(format).setInitialLayout(initialLayout).setFinalLayout(finalLayout).setLoadOp(loadOp).setStoreOp(storeOp).setStencilLoadOp(stencilLoadOp).setStencilStoreOp(stencilStoreOp);
	m_Data->m_ColorAttachments[id] = { id, image, attachment, clearColor, subpassLayout };
	return *this;
}

RenderTarget& egx::RenderTarget::AddColorAttachment(int32_t id, const Image2D& image, vk::Format format,
	vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
	vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
	vk::ClearValue clearColor, vk::AttachmentLoadOp stencilLoadOp,
	vk::AttachmentStoreOp stencilStoreOp)
{
	if (m_Data->m_ColorAttachments.contains(id))
	{
		throw runtime_error(cpp::Format("Color Attachment already exists with id {}", id));
	}
	vk::AttachmentDescription2 attachment;
	attachment.setFormat(format).setInitialLayout(initialLayout).setFinalLayout(finalLayout).setLoadOp(loadOp).setStoreOp(storeOp).setStencilLoadOp(stencilLoadOp).setStencilStoreOp(stencilStoreOp);
	m_Data->m_ColorAttachments[id] = { id, image, attachment, clearColor, subpassLayout };
	return *this;
}

RenderTarget& egx::RenderTarget::CreateSingleDepthAttachment(vk::Format format,
	vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
	vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
	vk::ClearValue clearValue, vk::AttachmentLoadOp stencilLoadOp,
	vk::AttachmentStoreOp stencilStoreOp)
{
	throw runtime_error("Operation not implemented.");
	return *this;
}

void egx::RenderTarget::Invalidate()
{
	LOG(WARNING, "(TODO) Handle Errors in RenterTarget::Invalidate()");
	// 1) Create data structures
	vector<AttachmentDescription2> attachments;
	vector<AttachmentReference2> colorAttachmentReferences;
	vector<ImageView> imageViews;
	AttachmentReference2 depthAttachmentReference;
	int colorAttachmentIndex = m_SwapchainFlag ? 1 : 0;
	if (m_SwapchainFlag) {
		const auto& attachment = m_Data->m_ColorAttachments[-1];
		imageViews.push_back(nullptr);
		attachments.push_back(attachment.Description);
		colorAttachmentReferences.push_back(AttachmentReference2(0, attachment.SubpassLayout, GetFormatAspectFlags(attachment.Description.format)));
	}
	if (m_Data->m_DepthAttachment)
	{
		attachments.push_back(m_Data->m_DepthAttachment->Description);
		depthAttachmentReference = AttachmentReference2(0, m_Data->m_DepthAttachment->SubpassLayout, GetFormatAspectFlags(m_Data->m_DepthAttachment->Description.format));
		if (m_Data->m_DepthAttachment->Image.ContainsView(0))
		{
			m_Data->m_DepthAttachment->Image.CreateView(0);
		}
		imageViews.push_back(m_Data->m_DepthAttachment->Image.GetView(0));
		colorAttachmentIndex++;
	}
	for (auto& [id, attachment] : m_Data->m_ColorAttachments)
	{
		if (id < 0 && m_SwapchainFlag) {
			continue;
		}
		attachments.push_back(attachment.Description);
		colorAttachmentReferences.push_back(AttachmentReference2(colorAttachmentIndex, attachment.SubpassLayout, GetFormatAspectFlags(attachment.Description.format)));
		if (!attachment.Image.ContainsView(0))
		{
			attachment.Image.CreateView(0);
		}
		imageViews.push_back(attachment.Image.GetView(0));
		colorAttachmentIndex++;
	}

	// 2) Create Render Pass
	SubpassDescription2 subpass;
	subpass.setPipelineBindPoint(PipelineBindPoint::eGraphics)
		.setColorAttachments(colorAttachmentReferences);

	if (m_Data->m_DepthAttachment)
		subpass.setPDepthStencilAttachment(&depthAttachmentReference);

	RenderPassCreateInfo2 createInfo;
	createInfo.setAttachments(attachments).setSubpasses(subpass);
	m_Data->m_RenderPass = m_Data->m_Ctx->Device.createRenderPass2(createInfo);
	// 3) Create framebuffer
	vk::FramebufferCreateInfo framebufferCreateInfo;
	framebufferCreateInfo.setRenderPass(m_Data->m_RenderPass)
		.setAttachments(imageViews)
		.setWidth(m_Data->m_Width)
		.setHeight(m_Data->m_Height)
		.setLayers(1);

	if (m_SwapchainFlag) {
		auto backBufferCount = ranges::count_if(m_Data->m_ColorAttachments, [](auto& x) -> bool { return x.first < 0; });

		for (int32_t i = -1; i >= -backBufferCount; i--) {
			auto& attachment = m_Data->m_ColorAttachments[i];
			if (!attachment.Image.ContainsView(0))
				attachment.Image.CreateView(0);
			imageViews[0] = attachment.Image.GetView(0);
			m_Data->m_SwapchainFramebuffers.push_back(m_Data->m_Ctx->Device.createFramebuffer(framebufferCreateInfo));
		}
	}
	else {
		m_Data->m_Framebuffer = m_Data->m_Ctx->Device.createFramebuffer(framebufferCreateInfo);
	}
}

Image2D egx::RenderTarget::GetAttachment(int32_t id)
{
	return m_Data->m_ColorAttachments.at(id).Image;
}

vk::AttachmentDescription2 egx::RenderTarget::GetAttachmentDescription(int32_t id)
{
	return m_Data->m_ColorAttachments.at(id).Description;
}

void RenderTarget::Begin(vk::CommandBuffer cmd)
{
	vk::RenderPassBeginInfo beginInfo;
	vector<vk::ClearValue> clearValues;
	if (m_Data->m_DepthAttachment)
	{
		if (m_Data->m_DepthAttachment->Description.loadOp == vk::AttachmentLoadOp::eClear)
		{
			clearValues.push_back(m_Data->m_DepthAttachment->Clear);
		}
	}
	if (m_SwapchainFlag) {
			clearValues.push_back(m_Data->m_ColorAttachments[-1].Clear);
	}
	for (auto& [id, attachment] : m_Data->m_ColorAttachments)
	{
		if (id < 0 && m_SwapchainFlag) {
			continue;
		}
		if (attachment.Description.loadOp == vk::AttachmentLoadOp::eClear)
		{
			clearValues.push_back(attachment.Clear);
		}
	}
	beginInfo.setRenderPass(m_Data->m_RenderPass).setRenderArea(vk::Rect2D().setExtent({ m_Data->m_Width, m_Data->m_Height })).setClearValues(clearValues);
	if (m_SwapchainFlag) {
		beginInfo.setFramebuffer(m_Data->m_SwapchainFramebuffers[m_Data->m_Ctx->CurrentFrame]);
	}
	else {
		beginInfo.setFramebuffer(m_Data->m_Framebuffer);
	}
	vk::SubpassBeginInfo subpassBegin;
	subpassBegin.setContents(vk::SubpassContents::eInline);
	cmd.beginRenderPass2(beginInfo, subpassBegin);
}

void RenderTarget::End(vk::CommandBuffer cmd)
{
	vk::SubpassEndInfo endInfo;
	cmd.endRenderPass2(endInfo);
}

RenderTarget::DataWrapper::~DataWrapper()
{
	if (m_Ctx && m_RenderPass)
	{
		m_Ctx->Device.destroyRenderPass(m_RenderPass);
		m_Ctx->Device.destroyFramebuffer(m_Framebuffer);
	}
}