#include "RenderTarget.hpp"
#include <memory/formatsize.hpp>

using namespace std;
using namespace egx;
using namespace vk;

egx::IRenderTarget::IRenderTarget(const DeviceCtx& ctx, uint32_t width, uint32_t height)
{
	m_Data = make_shared<IRenderTarget::DataWrapper>();
	m_Data->m_Ctx = ctx, m_Data->m_Width = width, m_Data->m_Height = height;
}

egx::IRenderTarget::IRenderTarget(const DeviceCtx& ctx, const ISwapchainController& swapchain, vk::ImageLayout initialLayout, 
	vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout, vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, 
	vk::ClearValue clearColor, vk::AttachmentLoadOp stencilLoadOp, vk::AttachmentStoreOp stencilStoreOp)
	: IRenderTarget(ctx, swapchain.Width(), swapchain.Height())
{
	m_Data->m_SwapchainFlag = true;
	m_Data->m_Swapchain = swapchain;
	m_Data->swapchain_info.initialLayout = initialLayout;
	m_Data->swapchain_info.subpassLayout = subpassLayout;
	m_Data->swapchain_info.finalLayout = finalLayout;
	m_Data->swapchain_info.loadOp = loadOp;
	m_Data->swapchain_info.storeOp = storeOp;
	m_Data->swapchain_info.clearColor = clearColor;
	m_Data->swapchain_info.stencilLoadOp = stencilLoadOp;
	m_Data->swapchain_info.stencilStoreOp = stencilStoreOp;
	FetchSwapchainBackBuffers();
	m_Data->m_Swapchain.AddResizeCallback(this, nullptr);
}

void egx::IRenderTarget::CallbackProtocol(void* pUserData)
{
	FetchSwapchainBackBuffers();
	Invalidate();
}

std::unique_ptr<IUniqueHandle> egx::IRenderTarget::MakeHandle() const
{
	unique_ptr<IRenderTarget> rt = make_unique<IRenderTarget>();
	rt->m_Data = m_Data;
	return rt;
}

void egx::IRenderTarget::FetchSwapchainBackBuffers()
{
	m_Data->m_Width = m_Data->m_Swapchain.Width(), m_Data->m_Height = m_Data->m_Swapchain.Height();
	auto backBuffers = m_Data->m_Swapchain.GetBackBuffers();
	int32_t i = -1;
	for (vk::Image handle : backBuffers)
	{
		auto image = Image2D::CreateFromHandle(m_Data->m_Ctx, handle, m_Data->m_Swapchain.Width(), m_Data->m_Swapchain.Height(), m_Data->m_Swapchain.GetFormat(), 1, vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eUndefined);
		AddColorAttachment(i, image, image.Format, m_Data->swapchain_info.initialLayout, m_Data->swapchain_info.subpassLayout,
			m_Data->swapchain_info.finalLayout, m_Data->swapchain_info.loadOp, m_Data->swapchain_info.storeOp, m_Data->swapchain_info.clearColor,
			m_Data->swapchain_info.stencilLoadOp, m_Data->swapchain_info.stencilStoreOp);
		i--;
	}
}

IRenderTarget& egx::IRenderTarget::CreateColorAttachment(int32_t id, vk::Format format,
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

IRenderTarget& egx::IRenderTarget::AddColorAttachment(int32_t id, const Image2D& image, vk::Format format,
	vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
	vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
	vk::ClearValue clearColor, vk::AttachmentLoadOp stencilLoadOp,
	vk::AttachmentStoreOp stencilStoreOp)
{
	if (m_Data->m_ColorAttachments.contains(id) && id > 0)
	{
		throw runtime_error(cpp::Format("Color Attachment already exists with id {}", id));
	}
	vk::AttachmentDescription2 attachment;
	attachment.setFormat(format).setInitialLayout(initialLayout).setFinalLayout(finalLayout).setLoadOp(loadOp).setStoreOp(storeOp).setStencilLoadOp(stencilLoadOp).setStencilStoreOp(stencilStoreOp);
	m_Data->m_ColorAttachments[id] = { id, image, attachment, clearColor, subpassLayout };
	return *this;
}

IRenderTarget& egx::IRenderTarget::CreateSingleDepthAttachment(vk::Format format,
	vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
	vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
	vk::ClearDepthStencilValue clearValue, vk::AttachmentLoadOp stencilLoadOp,
	vk::AttachmentStoreOp stencilStoreOp)
{
	Image2D image(m_Data->m_Ctx, m_Data->m_Width, m_Data->m_Height, format, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eUndefined, false);
	vk::AttachmentDescription2 attachment;
	attachment.setFormat(format).setInitialLayout(initialLayout).setFinalLayout(finalLayout).setLoadOp(loadOp).setStoreOp(storeOp).setStencilLoadOp(stencilLoadOp).setStencilStoreOp(stencilStoreOp);
	m_Data->m_DepthAttachment = { -1, image, attachment, clearValue, subpassLayout };
	return *this;
}

IRenderTarget& egx::IRenderTarget::EnableDearImGui(const PlatformWindow& window)
{
	if (m_Data->m_DearImGuiFlag) return *this;
	m_Data->m_DearImGuiFlag = true;
	m_Data->m_WindowPtr = window.GetWindow();
	return *this;
}

ImGuiContext* egx::IRenderTarget::GetImGuiContext()
{
	if (!m_Data->m_DearImGuiFlag) return nullptr;
	return m_Data->m_ImGuiController.GetContext();
}

void egx::IRenderTarget::Invalidate()
{
	LOG(WARNING, "(TODO) Handle Errors in RenterTarget::Invalidate()");
	m_Data->Reinvalidate();

	// 1) Create data structures
	vector<AttachmentDescription2> attachments;
	vector<AttachmentReference2> colorAttachmentReferences;
	vector<ImageView> imageViews;
	AttachmentReference2 depthAttachmentReference;
	int colorAttachmentIndex = m_Data->m_SwapchainFlag ? 1 : 0;
	if (m_Data->m_SwapchainFlag) {
		const auto& attachment = m_Data->m_ColorAttachments[-1];
		imageViews.push_back(nullptr);
		attachments.push_back(attachment.Description);
		colorAttachmentReferences.push_back(AttachmentReference2(0, attachment.SubpassLayout, GetFormatAspectFlags(attachment.Description.format)));
	}
	if (m_Data->m_DepthAttachment)
	{
		attachments.push_back(m_Data->m_DepthAttachment->Description);
		depthAttachmentReference = AttachmentReference2(colorAttachmentIndex, m_Data->m_DepthAttachment->SubpassLayout, GetFormatAspectFlags(m_Data->m_DepthAttachment->Description.format));
		if (!m_Data->m_DepthAttachment->Image.ContainsView(0))
		{
			m_Data->m_DepthAttachment->Image.CreateView(0);
		}
		imageViews.push_back(m_Data->m_DepthAttachment->Image.GetView(0));
		colorAttachmentIndex++;
	}
	for (auto& [id, attachment] : m_Data->m_ColorAttachments)
	{
		if (id < 0 && m_Data->m_SwapchainFlag) {
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

	// Extracted from ImGui vulkan backend
	VkSubpassDependency2 dependency = { VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2 };
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	RenderPassCreateInfo2 createInfo;
	createInfo.setAttachments(attachments).setSubpasses(subpass);

	if (m_Data->m_DearImGuiFlag) {
		// createInfo.setDependencyCount(1).setPDependencies((SubpassDependency2*)&dependency);
	}

	m_Data->m_RenderPass = m_Data->m_Ctx->Device.createRenderPass2(createInfo);

	// initialize DearImGui
	if (m_Data->m_DearImGuiFlag) {
		// clear old object
		m_Data->m_ImGuiController = {};
		// create new object
		m_Data->m_ImGuiController = DearImGuiController(m_Data->m_Ctx, m_Data->m_WindowPtr, m_Data->m_RenderPass);
	}

	// 3) Create framebuffer
	vk::FramebufferCreateInfo framebufferCreateInfo;
	framebufferCreateInfo.setRenderPass(m_Data->m_RenderPass)
		.setAttachments(imageViews)
		.setWidth(m_Data->m_Width)
		.setHeight(m_Data->m_Height)
		.setLayers(1);

	if (m_Data->m_SwapchainFlag) {
		auto backBufferCount = ranges::count_if(m_Data->m_ColorAttachments, [](auto& x) -> bool { return x.first < 0; });

		for (int32_t i = -1; i >= -backBufferCount; i--) {
			auto& attachment = m_Data->m_ColorAttachments[i];
			if (!attachment.Image.ContainsView(0))
				attachment.Image.CreateView(0);
			// Updating imageViews updates the framebufferCreateInfo
			imageViews[0] = attachment.Image.GetView(0);
			m_Data->m_SwapchainFramebuffers.push_back(m_Data->m_Ctx->Device.createFramebuffer(framebufferCreateInfo));
		}
	}
	else {
		m_Data->m_Framebuffer = m_Data->m_Ctx->Device.createFramebuffer(framebufferCreateInfo);
	}
}

Image2D egx::IRenderTarget::GetAttachment(int32_t id)
{
	return m_Data->m_ColorAttachments.at(id).Image;
}

vk::AttachmentDescription2 egx::IRenderTarget::GetAttachmentDescription(int32_t id)
{
	return m_Data->m_ColorAttachments.at(id).Description;
}

void IRenderTarget::Begin(vk::CommandBuffer cmd)
{
	vk::RenderPassBeginInfo beginInfo;
	vector<vk::ClearValue> clearValues;
	if (m_Data->m_SwapchainFlag) {
		clearValues.push_back(m_Data->m_ColorAttachments[-1].Clear);
	}
	if (m_Data->m_DepthAttachment)
	{
		if (m_Data->m_DepthAttachment->Description.loadOp == vk::AttachmentLoadOp::eClear)
		{
			clearValues.push_back(m_Data->m_DepthAttachment->Clear);
		}
	}
	for (auto& [id, attachment] : m_Data->m_ColorAttachments)
	{
		if (id < 0 && m_Data->m_SwapchainFlag) {
			continue;
		}
		if (attachment.Description.loadOp == vk::AttachmentLoadOp::eClear)
		{
			clearValues.push_back(attachment.Clear);
		}
	}
	beginInfo.setRenderPass(m_Data->m_RenderPass).setRenderArea(vk::Rect2D().setExtent({ m_Data->m_Width, m_Data->m_Height })).setClearValues(clearValues);
	if (m_Data->m_SwapchainFlag) {
		beginInfo.setFramebuffer(m_Data->m_SwapchainFramebuffers[m_Data->m_Ctx->CurrentFrame]);
	}
	else {
		beginInfo.setFramebuffer(m_Data->m_Framebuffer);
	}
	vk::SubpassBeginInfo subpassBegin;
	subpassBegin.setContents(vk::SubpassContents::eInline);
	cmd.beginRenderPass2(beginInfo, subpassBegin);
}

void IRenderTarget::End(vk::CommandBuffer cmd)
{
	vk::SubpassEndInfo endInfo;
	cmd.endRenderPass2(endInfo);
}

void egx::IRenderTarget::BeginDearImguiFrame()
{
	if (m_Data->m_DearImGuiFlag) {
		m_Data->m_ImGuiController.BeginFrame();
	}
}

void egx::IRenderTarget::EndDearImguiFrame(vk::CommandBuffer cmd)
{
	if (m_Data->m_DearImGuiFlag) {
		m_Data->m_ImGuiController.EndFrame(cmd);
	}
}

IRenderTarget::DataWrapper::~DataWrapper()
{
	Reinvalidate();
}

void IRenderTarget::DataWrapper::Reinvalidate() {
	if (m_Ctx && m_RenderPass)
	{
		m_Ctx->Device.destroyRenderPass(m_RenderPass);
		for (vk::Framebuffer framebuffer : m_SwapchainFramebuffers)
			m_Ctx->Device.destroyFramebuffer(framebuffer);
		m_Ctx->Device.destroyFramebuffer(m_Framebuffer);
		m_RenderPass = nullptr, m_Framebuffer = nullptr;
		m_SwapchainFramebuffers.clear();
	}
}