#include "egxframebuffer.hpp"

egx::ref<egx::egxframebuffer>  egx::egxframebuffer::FactoryCreate(ref<VulkanCoreInterface>& CoreInterface, uint32_t Width, uint32_t Height)
{
	return ref<egxframebuffer>(new egxframebuffer(CoreInterface, Width, Height));
}

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
	_internal::egfxframebuffer_attachment attachment;
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

/*
void EGX_API egx::egxframebuffer::start() {
	if (_renderpass == nullptr) {
		std::vector<VkAttachmentDescription> attachments;
		for (auto&[index, a] : _colorattachements) {
			VkAttachmentDescription attachment{};
			attachment.flags = 0;
			attachment.format = a.Attachment->Format;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = a.AttachmentInfo.loadOp;
			attachment.storeOp = a.AttachmentInfo.storeOp;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout;
			attachment.finalLayout;
			attachments.push_back(attachment);
		}
		VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		createInfo.attachmentCount = attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount;
		createInfo.pSubpasses;
		vkCreateRenderPass(_coreinterface->Device, &createInfo, nullptr, &_renderpass);
	}
}

void EGX_API egx::egxframebuffer::end() {

}
*/
