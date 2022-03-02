#pragma once
#include <vector>
#include <map>
#include <vulkan/vulkan_core.h>
#include <memory/Texture2.hpp>
#include <backend/VkGraphicsCard.hpp>

class FramebufferAttachment {
public:
	FramebufferAttachment(VkFormat format, VkClearColorValue clearColor, VkPipelineColorBlendAttachmentState* pBlendState = nullptr)
		: m_format(format), m_clear_color(clearColor)
	{
		if (pBlendState) {
			m_blend_state = *pBlendState;
			return;
		}
		m_blend_state = {};
		m_blend_state.blendEnable = VK_FALSE;
		m_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	FramebufferAttachment(VkFormat format, VkClearColorValue clearColor, ITexture2 attachment, VkPipelineColorBlendAttachmentState* pBlendState = nullptr)
		: m_format(format), m_clear_color(clearColor), m_attachment(attachment)
	{
		if (pBlendState) {
			m_blend_state = *pBlendState;
			return;
		}
		m_blend_state = {};
		m_blend_state.blendEnable = VK_FALSE;
		m_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	bool IsDepthAttachment() {
		return (m_format == VK_FORMAT_D16_UNORM) ||
			(m_format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
			(m_format == VK_FORMAT_D32_SFLOAT) ||
			(m_format == VK_FORMAT_D16_UNORM_S8_UINT) ||
			(m_format == VK_FORMAT_D24_UNORM_S8_UINT) ||
			(m_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
	}

	bool IsStencilAttachment() {
		return (m_format == VK_FORMAT_D16_UNORM_S8_UINT) ||
			(m_format == VK_FORMAT_D24_UNORM_S8_UINT) ||
			(m_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
	}

	inline void SetAttachment(ITexture2 attachment) { m_attachment = attachment; }

	inline const VkFormat GetFormat() { return m_format; }
	inline const VkPipelineColorBlendAttachmentState GetBlendState() { return m_blend_state; }
	inline const ITexture2 GetAttachment() { return m_attachment; }

public:
	VkClearColorValue m_clear_color;

private:
	VkFormat m_format;
	VkPipelineColorBlendAttachmentState m_blend_state;
	ITexture2 m_attachment;
};

struct Framebuffer {
	uint32_t m_width, m_height;
	std::map<uint32_t, FramebufferAttachment*> m_color_attachments;
	FramebufferAttachment* m_depth_attachment;

	void AddColorAttachment(uint32_t attachmentIndex, FramebufferAttachment* attachment, ITexture2 texture) {
		m_color_attachments.insert(std::make_pair(attachmentIndex, attachment));
	}

	void SetDepthAttachment(FramebufferAttachment* attachment, ITexture2 texture) {
		m_depth_attachment = attachment;
	}

};

/*
	Since were using dynamic rendering (no render pass) we must transistion textures into "final layout" in the first use
*/

void Framebuffer_TransistionAttachment(VkCommandBuffer cmd, FramebufferAttachment* attachment, VkAccessFlags dstAccess, VkImageLayout newLayout,
	VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	VkAccessFlags srcAccess = VK_ACCESS_NONE,
	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
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


void Framebuffer_TransistionImage(VkCommandBuffer cmd, ITexture2 attachment, VkImageAspectFlags aspect, uint32_t frameIndex, VkAccessFlags dstAccess, VkImageLayout newLayout,
	VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	VkAccessFlags srcAccess = VK_ACCESS_NONE,
	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
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

void Framebuffer_TransistionImage(VkCommandBuffer cmd, FramebufferAttachment* attachment, uint32_t frameIndex, VkAccessFlags dstAccess, VkImageLayout newLayout,
	VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	VkAccessFlags srcAccess = VK_ACCESS_NONE,
	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
{
	assert(attachment);
	assert(attachment->GetAttachment());
	VkImageAspectFlags aspect = attachment->IsDepthAttachment() ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	aspect |= attachment->IsStencilAttachment() ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
	Framebuffer_TransistionImage(cmd, attachment->GetAttachment(), aspect, frameIndex, dstAccess, newLayout, oldLayout, srcAccess, srcStage, dstStage);
}
