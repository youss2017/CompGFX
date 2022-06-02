#pragma once
#include <EngineBase.h>
#include <backend/backend_base.h>
#include <memory/vulkan_memory_allocator.hpp>

struct TextureMipmap {
	std::vector<std::vector<VkImageView>> mMipmapViewsPerFrame;
};

struct GPUTexture2D_2
{
	vk::VkContext m_context;
	Texture2DSpecification m_specification;
	VkAlloc::IMAGE_DESCRIPITION m_vk_description;
	VkAlloc::IMAGE m_vk_image;
	VkImageView m_vk_view;
	std::vector<VkImage> m_vk_images_per_frame;
	std::vector<VkImageView> m_vk_views_per_frame;
	VkImageAspectFlags m_vk_aspectmask;
	VkImageLayout m_vk_current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout m_vk_current_layout_mipmap = VK_IMAGE_LAYOUT_UNDEFINED;
	uint32_t mMipCount;
	TextureMipmap mMipmapViews;
};

typedef GPUTexture2D_2* ITexture2;

ITexture2 GRAPHICS_API Texture2_Create(const Texture2DSpecification& specification);
void GRAPHICS_API Texture2_UploadPixels(ITexture2 texture, void* pixels, uint32_t size);
void GRAPHICS_API Texture2_UpdateMipmaps(ITexture2 texture);
// PixelSize refers to the size of a single pixel, what access do you want after copy is complete
void GRAPHICS_API Texture2_ReadPixels(ITexture2 texture, VkAccessFlagBits finalAccessFlags, VkImageLayout currentLayout, uint32_t pixelSize, void* buffer);
void GRAPHICS_API Texture2_Destroy(ITexture2 texture);

ITexture2 GRAPHICS_API Texture2_CreateFromFile(const char* path, bool mipmaps);
void GRAPHICS_API Texture2_RemoveAlphaChannel(uint32_t* srcPixels, char* dstPixels, int width, int height);

// Converts "RGBA8" to TextureFormat::RGBA8
//TextureFormat Textures_StringToTextureFormat(std::string& input);

