#pragma once
#include <EngineBase.h>
#include <backend/backend_base.h>
#include <memory/vulkan_memory_allocator.hpp>

struct GPUTexture2D_2
{
	GraphicsContext m_context;
	Texture2DSpecification m_specification;
	VkAlloc::IMAGE_DESCRIPITION m_vk_description;
	VkImageView m_vk_view;
	VkAlloc::IMAGE m_vk_image;
	std::vector<VkImageView> m_vk_views;
	std::vector<VkAlloc::IMAGE> m_vk_images;
	VkImageAspectFlags m_vk_aspectmask;
	VkImageLayout m_vk_current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout m_vk_current_layout_mipmap = VK_IMAGE_LAYOUT_UNDEFINED;
};

typedef GPUTexture2D_2* ITexture2;

ITexture2 Texture2_Create(GraphicsContext context, const Texture2DSpecification& specification);
void Texture2_UploadPixels(ITexture2 texture, void* pixels, uint32_t size);
void Texture2_UpdateMipmaps(ITexture2 texture);
void Texture2_Destroy(ITexture2 texture);

ITexture2 Texture2_CreateFromFile(GraphicsContext context, const char* path, bool mipmaps);
