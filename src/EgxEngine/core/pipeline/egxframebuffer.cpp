#include "egxframebuffer.hpp"
#include <stdexcept>
#include <Utility/CppUtility.hpp>
#include "../memory/formatsize.hpp"

egx::ref<egx::Framebuffer>  egx::Framebuffer::FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface, uint32_t Width, uint32_t Height)
{
	return ref<Framebuffer>(new Framebuffer(CoreInterface, Width, Height));
}

// Dynmaic Rendering
#if 0
void  egx::egxframebuffer::CreateColorAttachment(
	uint32_t ColorAttachmentID,
	VkFormat Format,
	VkClearValue ClearValue,
	VkImageUsageFlags CustomUsageFlags,
	VkImageLayout ImageLayout,
	VkAttachmentLoadOp LoadOp,
	VkAttachmentStoreOp StoreOp,
	VkPipelineColorBlendAttachmentState* pBlendState) {
	assert(_colorattachements.find(ColorAttachmentID) == _colorattachements.end());
	internal::egfxframebuffer_attachment attachment;
	if (pBlendState)
		attachment.BlendState = *pBlendState;
	attachment.Attachment = Image::FactoryCreate(
		_coreinterface,
		memorylayout::local,
		VK_IMAGE_ASPECT_COLOR_BIT,
		Width,
		Height,
		1,
		Format,
		1,
		1,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CustomUsageFlags,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	attachment.View = attachment.Attachment->createview(0);

	attachment.AttachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	attachment.AttachmentInfo.imageView = attachment.View;
	attachment.AttachmentInfo.imageLayout = ImageLayout;
	attachment.AttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
	attachment.AttachmentInfo.resolveImageView = nullptr;
	attachment.AttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.AttachmentInfo.loadOp = LoadOp;
	attachment.AttachmentInfo.storeOp = StoreOp;
	attachment.AttachmentInfo.clearValue = ClearValue;

	_colorattachements.insert({ ColorAttachmentID, attachment });
}

void  egx::egxframebuffer::CreateDepthAttachment(
	VkFormat Format,
	VkClearValue ClearValue,
	VkImageUsageFlags CustomUsageFlags,
	VkImageLayout ImageLayout,
	VkAttachmentLoadOp LoadOp,
	VkAttachmentStoreOp StoreOp)
{
	assert(!_depthattachment.has_value());
	_internal::egfxframebuffer_attachment attachment{};
	attachment.Attachment = Image::FactoryCreate(
		_coreinterface,
		memorylayout::local,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		Width,
		Height,
		1,
		Format,
		1,
		1,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CustomUsageFlags,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	attachment.View = attachment.Attachment->createview(0);

	attachment.AttachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	attachment.AttachmentInfo.imageView = attachment.View;
	attachment.AttachmentInfo.imageLayout = ImageLayout;
	attachment.AttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
	attachment.AttachmentInfo.resolveImageView = nullptr;
	attachment.AttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.AttachmentInfo.loadOp = LoadOp;
	attachment.AttachmentInfo.storeOp = StoreOp;
	attachment.AttachmentInfo.clearValue = ClearValue;

	_depthattachment = attachment;
}
#endif

namespace egx {

	Framebuffer::~Framebuffer() {
		if (_renderpass) vkDestroyRenderPass(_coreinterface->Device, _renderpass, nullptr);
		if (_framebuffer) vkDestroyFramebuffer(_coreinterface->Device, _framebuffer, nullptr);
	}

	void Framebuffer::CreateColorAttachment(
		uint32_t ColorAttachmentId,
		VkFormat Format,
		VkClearValue ClearValue,
		VkAttachmentLoadOp LoadOp,
		VkAttachmentStoreOp StoreOp,
		VkImageLayout InitialLayout,
		VkImageLayout FinalLayout,
		VkImageUsageFlags CustomUsageFlags,
		bool CreateDefaultView,
		VkPipelineColorBlendAttachmentState* pBlendState)
	{
		assert(_renderpass == nullptr);
		assert(_colorattachements.find(ColorAttachmentId) == _colorattachements.end());
#ifdef _DEBUG
		LOG(INFO, "Creating FBO Color Attachment {0}x{1} Format: {2} Clear: <{3}, {4}, {5}, {6}>", 
			Width, Height, egx::_internal::DebugVkFormatToString(Format),
			ClearValue.color.float32[0], ClearValue.color.float32[1], ClearValue.color.float32[2], ClearValue.color.float32[3]);
#endif
		internal::egfxframebuffer_attachment attachment;
		if (pBlendState)
			attachment.BlendState = *pBlendState;
		attachment.Attachment = Image::FactoryCreate(
			_coreinterface,
			memorylayout::local,
			VK_IMAGE_ASPECT_COLOR_BIT,
			Width,
			Height,
			1,
			Format,
			1,
			1,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CustomUsageFlags,
			InitialLayout);

		if (CreateDefaultView)
			attachment.View = attachment.Attachment->createview(0);

		attachment.Description.format = Format;
		attachment.Description.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.Description.loadOp = LoadOp;
		attachment.Description.storeOp = StoreOp;
		attachment.Description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.Description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.Description.initialLayout = InitialLayout;
		attachment.Description.finalLayout = FinalLayout;
		attachment.ClearValue = ClearValue;

		_colorattachements.insert({ ColorAttachmentId, attachment });
	}

	// Same as CreateColorAttachment
	void Framebuffer::CreateDepthAttachment(
		VkFormat Format,
		VkClearValue ClearValue,
		VkAttachmentLoadOp LoadOp,
		VkAttachmentStoreOp StoreOp,
		VkImageLayout InitialLayout,
		VkImageLayout FinalLayout,
		VkImageUsageFlags CustomUsageFlags,
		bool CreateDefaultView)
	{
		assert(_renderpass == nullptr);
		assert(!_depthattachment.has_value());
#ifdef _DEBUG
		LOG(INFO, "Creating FBO Depth Attachment {0}x{1} Format: {2}, Clear: {3}", Width, Height, egx::_internal::DebugVkFormatToString(Format), ClearValue.depthStencil.depth);
#endif
		internal::egfxframebuffer_attachment attachment;
		attachment.Attachment = Image::FactoryCreate(
			_coreinterface,
			memorylayout::local,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			Width,
			Height,
			1,
			Format,
			1,
			1,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CustomUsageFlags,
			InitialLayout);

		if (CreateDefaultView)
			attachment.View = attachment.Attachment->createview(0);

		attachment.Description.format = Format;
		attachment.Description.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.Description.loadOp = LoadOp;
		attachment.Description.storeOp = StoreOp;
		attachment.Description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.Description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.Description.initialLayout = InitialLayout;
		attachment.Description.finalLayout = FinalLayout;
		attachment.ClearValue = ClearValue;

		_depthattachment = attachment;
	}

	uint32_t Framebuffer::CreateSubpass(
		const std::vector<std::pair<uint32_t, VkImageLayout>>& ColorAttachmentIds,
		std::optional<VkImageLayout> DepthAttachment,
		const std::vector<std::pair<uint32_t, VkImageLayout>>& InputAttachments)
	{
		assert(_renderpass == nullptr);
		if (DepthAttachment.has_value())
			assert(_depthattachment.has_value() && _depthattachment->Attachment.IsValidRef());
		assert(ColorAttachmentIds.size() > 0 || DepthAttachment.has_value());

		int inputK = 0;
		int colorK = 0;
		int depthK = 0;
		int depthAttachmentOffset = _depthattachment.has_value() ? 1 : 0;

		inputK = (int)_attachment_ref.size();
		for (auto& [id, layout] : InputAttachments) {
			if (_colorattachements.find(id) == _colorattachements.end()) {
				throw std::runtime_error("Could not find input attachment specified Id");
			}
			_attachment_ref.push_back({
				(uint32_t)std::distance(_colorattachements.begin(), _colorattachements.find(id)) + depthAttachmentOffset
				, layout });
		}

		colorK = (int)_attachment_ref.size();
		for (auto& [id, layout] : ColorAttachmentIds) {
			if (_colorattachements.find(id) == _colorattachements.end()) {
				throw std::runtime_error("Could not find color attachment specified Id");
			}
			_attachment_ref.push_back({
				(uint32_t)std::distance(_colorattachements.begin(), _colorattachements.find(id)) + depthAttachmentOffset
				, layout });
		}

		depthK = (int)_attachment_ref.size();
		if (DepthAttachment.has_value()) {
			_attachment_ref.push_back({ 0, DepthAttachment.value() });
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = (uint32_t)InputAttachments.size();
		subpass.pInputAttachments = &_attachment_ref[inputK];
		subpass.colorAttachmentCount = (uint32_t)ColorAttachmentIds.size();
		subpass.pColorAttachments = &_attachment_ref[colorK];
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = DepthAttachment.has_value() ? &_attachment_ref[depthK] : nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;
		_subpass.push_back(subpass);
		return (uint32_t)_subpass.size() - 1u;
	}

	EGX_API void Framebuffer::InternalCreate()
	{
		assert(_subpass.size() > 0 && "You must have at least 1 pass.");
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkImageView> attachmentViews;
		if (_depthattachment.has_value()) {
			attachments.push_back(_depthattachment->Description);
			attachmentViews.push_back(_depthattachment->Attachment->view(0));
		}
		for (auto& [id, attachment] : _colorattachements) {
			attachments.push_back(attachment.Description);
			attachmentViews.push_back(attachment.Attachment->view(0));
		}
		VkRenderPassCreateInfo passCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		passCreateInfo.attachmentCount = (uint32_t)attachments.size();
		passCreateInfo.pAttachments = attachments.data();
		passCreateInfo.subpassCount = (uint32_t)_subpass.size();
		passCreateInfo.pSubpasses = _subpass.data();
		passCreateInfo.dependencyCount = 0;
		passCreateInfo.pDependencies = nullptr;
		vkCreateRenderPass(_coreinterface->Device, &passCreateInfo, nullptr, &_renderpass);

		VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = _renderpass;
		createInfo.attachmentCount = (uint32_t)attachmentViews.size();
		createInfo.pAttachments = attachmentViews.data();
		createInfo.width = Width;
		createInfo.height = Height;
		createInfo.layers = 1;
		vkCreateFramebuffer(_coreinterface->Device, &createInfo, nullptr, &_framebuffer);
	}

}

