#pragma once
#include <vector>
#include <map>
#include <vulkan/vulkan_core.h>
#include <memory/Texture2.hpp>
#include <backend/VkGraphicsCard.hpp>
#include <optional>

class FramebufferAttachment {
public:
	static FramebufferAttachment GRAPHICS_API Create(VkImageUsageFlags usage, int width, int height, VkFormat format, VkClearValue clear, VkPipelineColorBlendAttachmentState* pBlendState = nullptr);
	
	void GRAPHICS_API Destroy() {
		Texture2_Destroy(m_attachment);
	}

	bool GRAPHICS_API IsDepthAttachment() {
		return (m_format == VK_FORMAT_D16_UNORM) ||
			(m_format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
			(m_format == VK_FORMAT_D32_SFLOAT) ||
			(m_format == VK_FORMAT_D16_UNORM_S8_UINT) ||
			(m_format == VK_FORMAT_D24_UNORM_S8_UINT) ||
			(m_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
	}

	bool GRAPHICS_API IsStencilAttachment() {
		return (m_format == VK_FORMAT_D16_UNORM_S8_UINT) ||
			(m_format == VK_FORMAT_D24_UNORM_S8_UINT) ||
			(m_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
	}

	inline const VkFormat GetFormat() { return m_format; }
	inline const VkPipelineColorBlendAttachmentState GetBlendState() { return m_blend_state; }
	inline const ITexture2 GetAttachment() { return m_attachment; }
	inline const VkImage GetImage(uint32_t FrameIndex) { return m_attachment->m_vk_images_per_frame[FrameIndex]; }
	inline const VkImageView GetView(uint32_t FrameIndex) { return m_attachment->m_vk_views_per_frame[FrameIndex]; }

public:
	VkClearValue mClear;
	uint32_t m_width, m_height;
	VkPipelineColorBlendAttachmentState m_blend_state;

private:
	VkFormat m_format;
	ITexture2 m_attachment;
};

struct Framebuffer {
	uint32_t m_width, m_height;
	std::map<uint32_t, FramebufferAttachment> m_color_attachments;
	std::optional<FramebufferAttachment> m_depth_attachment;

	void GRAPHICS_API AddColorAttachment(uint32_t attachmentIndex, FramebufferAttachment& attachment) {
		assert(!attachment.IsDepthAttachment() && !attachment.IsStencilAttachment());
		if (attachment.m_width != m_width || attachment.m_height != m_height) {
			log_error("Color attachment must be the same size as the framebuffer", __FILE__, __LINE__);
		}
		m_color_attachments.insert(std::make_pair(attachmentIndex, attachment));
	}

	void GRAPHICS_API SetDepthAttachment(FramebufferAttachment attachment) {
		assert(attachment.IsDepthAttachment());
		if (attachment.m_width != m_width || attachment.m_height != m_height) {
			log_error("Depth attachment must be the same size as the framebuffer", __FILE__, __LINE__);
		}
		m_depth_attachment = attachment;
	}

	// Do not call destory on any attachment used by this framebuffer
	void GRAPHICS_API DestroyAllBoundAttachments() {
		for (auto& col : m_color_attachments)
			col.second.Destroy();
		if (m_depth_attachment.has_value())
			m_depth_attachment.value().Destroy();
	}

	VkRenderingAttachmentInfo GRAPHICS_API GetRenderingAttachmentInfo(uint32_t FrameIndex, uint32_t colorAttachmentID, VkImageLayout layout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) {
		VkRenderingAttachmentInfo info{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR};
		info.imageView = m_color_attachments[colorAttachmentID].GetView(FrameIndex);
		info.imageLayout = layout;
		info.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
		info.resolveImageView = nullptr;
		info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.loadOp = loadOp;
		info.storeOp = storeOp;
		info.clearValue = m_color_attachments[colorAttachmentID].mClear;
		return info;
	}

	VkRenderingAttachmentInfo GRAPHICS_API GetDepthRenderingAttachmentInfo(uint32_t FrameIndex, VkImageLayout layout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) {
		VkRenderingAttachmentInfo info{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
		info.imageView = m_depth_attachment.value().GetView(FrameIndex);
		info.imageLayout = layout;
		info.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
		info.resolveImageView = nullptr;
		info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.loadOp = loadOp;
		info.storeOp = storeOp;
		info.clearValue = m_depth_attachment.value().mClear;
		return info;
	}

};
