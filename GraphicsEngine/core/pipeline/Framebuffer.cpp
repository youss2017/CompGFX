#include "Framebuffer.hpp"

FramebufferAttachment GRAPHICS_API FramebufferAttachment::Create(VkImageUsageFlags usage, int width, int height, VkFormat format, VkClearValue clear, VkPipelineColorBlendAttachmentState* pBlendState)
{
	FramebufferAttachment attachment;
	attachment.m_format = format;
	attachment.mClear = clear;
	attachment.m_width = width;
	attachment.m_height = height;
	if (!pBlendState) {
		attachment.m_blend_state = {};
		attachment.m_blend_state.blendEnable = VK_FALSE;
		attachment.m_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
	else {
		attachment.m_blend_state = *pBlendState;
	}
	Texture2DSpecification spec;
	spec.m_Format = format;
	spec.m_CreatePerFrame = true;
	spec.m_TextureUsage = attachment.IsDepthAttachment() ? (attachment.IsStencilAttachment() ? TextureUsage::DEPTH_STENCIL : TextureUsage::DEPTH_ONLY) : TextureUsage::COLOR;
	spec.m_GenerateMipMapLevels = false;
	spec.m_Width = width;
	spec.m_Height = height;
	spec.m_Samples = TextureSamples::MSAA_1;
	spec.mUsage = usage;
	attachment.m_attachment = Texture2_Create(spec);
	return attachment;
}

namespace vk {

	void GRAPHICS_API Framebuffer_TransistionAttachment(VkCommandBuffer cmd, ::FramebufferAttachment* attachment, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout,
		VkAccessFlags srcAccess,
		VkPipelineStageFlags srcStage,
		VkPipelineStageFlags dstStage)
	{
		assert(attachment);
		assert(attachment->GetAttachment());
		VkImageAspectFlags aspect = attachment->IsDepthAttachment() ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		aspect |= attachment->IsStencilAttachment() ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
		VkImageMemoryBarrier barrier[gFrameOverlapCount];
		for (int i = 0; i < gFrameOverlapCount; i++) {
			barrier[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier[i].pNext = nullptr;
			barrier[i].srcAccessMask = srcAccess;
			barrier[i].dstAccessMask = dstAccess;
			barrier[i].oldLayout = oldLayout;
			barrier[i].newLayout = newLayout;
			barrier[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier[i].image = attachment->GetAttachment()->m_vk_images_per_frame[i];
			barrier[i].subresourceRange.aspectMask = aspect;
			barrier[i].subresourceRange.baseMipLevel = 0;
			barrier[i].subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			barrier[i].subresourceRange.baseArrayLayer = 0;
			barrier[i].subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		}
		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, gFrameOverlapCount, barrier);
	}


	void GRAPHICS_API Framebuffer_TransistionImage(VkCommandBuffer cmd, ::ITexture2 attachment, VkImageAspectFlags aspect, uint32_t frameIndex, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout,
		VkAccessFlags srcAccess,
		VkPipelineStageFlags srcStage,
		VkPipelineStageFlags dstStage)
	{
		assert(attachment);
		VkImageMemoryBarrier barrier;
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = attachment->m_vk_images_per_frame[frameIndex];
		barrier.subresourceRange.aspectMask = aspect;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void GRAPHICS_API Framebuffer_TransistionImage(VkCommandBuffer cmd, ::FramebufferAttachment* attachment, uint32_t frameIndex, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout,
		VkAccessFlags srcAccess,
		VkPipelineStageFlags srcStage,
		VkPipelineStageFlags dstStage)
	{
		assert(attachment);
		assert(attachment->GetAttachment());
		VkImageAspectFlags aspect = attachment->IsDepthAttachment() ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		aspect |= attachment->IsStencilAttachment() ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
		Framebuffer_TransistionImage(cmd, attachment->GetAttachment(), aspect, frameIndex, dstAccess, newLayout, oldLayout, srcAccess, srcStage, dstStage);
	}

	void GRAPHICS_API Gfx_InsertDebugLabel(VkDevice device, VkCommandBuffer cmd, int FrameIndex, const std::string& debugLabelText, float r, float g, float b) {
#if defined(_DEBUG)
		std::string labelName = "[" + std::to_string(FrameIndex) + "]" + debugLabelText;
		VkDebugUtilsLabelEXT debugLabel{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		debugLabel.pLabelName = labelName.c_str();
		float debugColor[4] = { r, g, b, 1.0 };
		memcpy(&debugLabel.color[0], &debugColor[0], sizeof(float) * 4);
#ifndef VK_NO_PROTOTYPES
		static PFN_vkCmdBeginDebugUtilsLabelEXT beginUtilsLabelEXT = nullptr;
		if (!beginUtilsLabelEXT) {
			beginUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
		}
		beginUtilsLabelEXT(cmd, &debugLabel);
#else
		DvkCmdBeginDebugUtilsLabelEXT(cmd, &debugLabel);
#endif
#endif
	}

	void GRAPHICS_API Gfx_EndDebugLabel(VkDevice device, VkCommandBuffer cmd)
	{
#if defined(_DEBUG)
#ifndef VK_NO_PROTOTYPES
		static PFN_vkCmdEndDebugUtilsLabelEXT endUtilsLabelEXT = nullptr;
		if (!endUtilsLabelEXT) {
			endUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
		}
		endUtilsLabelEXT(cmd);
#else
		DvkCmdEndDebugUtilsLabelEXT(cmd);
#endif
#endif
	}

}