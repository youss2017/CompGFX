#pragma once
#include "../EngineBase.h"
#include <vector>

/******************************************************** ENGINE TEXTURE 2D ********************************************************/

struct GPUTexture2D {
	bool m_Vk_AllocatedMipMaps, m_CreatePerFrame;

	Texture2DSpecification m_specification;

	int m_ApiType;
	void *m_NativeHandle;
	std::vector<VkImage> m_images;	  // for CreatePerFrame
	std::vector<VkImageView> m_views; // for CreatePerFrame
	VkImageView view;
	VkDeviceMemory m_memory;
	VkDeviceSize m_MemoryOffset;
	uint32_t m_AllocationIndex;
	void *m_Context;
	uint32_t m_MipLevels;

	VkImageAspectFlags m_ImageAspectFlags = 0;
	VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout m_CurrentLayoutMips = VK_IMAGE_LAYOUT_UNDEFINED;
};

typedef GPUTexture2D *IGPUTexture2D;

typedef IGPUTexture2D PFN_GPUTexture2D_Create(GraphicsContext context, Texture2DSpecification *specification);
typedef void PFN_GPUTexture2D_UploadPixels(IGPUTexture2D texture, void *pixels, size_t pixel_size);
typedef void PFN_GPUTexture2D_GenerateMips(IGPUTexture2D texture);
typedef APIHandle PFN_GPUTexture2D_GetImage(IGPUTexture2D texture);
typedef APIHandle PFN_GPUTexture2D_GetView(IGPUTexture2D texture);
typedef void PFN_GPUTexture2D_Destroy(IGPUTexture2D texture);

extern PFN_GPUTexture2D_Create* GPUTexture2D_Create; 
extern PFN_GPUTexture2D_UploadPixels* GPUTexture2D_UploadPixels;
extern PFN_GPUTexture2D_GenerateMips* GPUTexture2D_GenerateMips;
extern PFN_GPUTexture2D_GetImage* GPUTexture2D_GetImage;
extern PFN_GPUTexture2D_GetView* GPUTexture2D_GetView;
extern PFN_GPUTexture2D_Destroy* GPUTexture2D_Destroy;

IGPUTexture2D GPUTexture2D_CreateFromFile(GraphicsContext context, const char* path, bool GenerateMipMaps);

void GPUTexture2D_LinkFunctions(GraphicsContext context);

struct GPUTextureSampler
{
	void *m_Context;
	void *m_NativeHandle;
	TextureSamplerSpecification m_Sampler;
};

typedef GPUTextureSampler *IGPUTextureSampler;

typedef IGPUTextureSampler PFN_GPUTextureSampler_Create(GraphicsContext context, TextureSamplerSpecification* specification);
typedef void PFN_GPUTextureSampler_Destroy(IGPUTextureSampler sampler);

extern PFN_GPUTextureSampler_Create* GPUTextureSampler_Create;
extern PFN_GPUTextureSampler_Destroy* GPUTextureSampler_Destroy;

IGPUTextureSampler GPUTextureSampler_CreateDefaultSampler(GraphicsContext context);

void GPUTextureSampler_LinkFunctions(GraphicsContext context);

// Converts "RGBA8" to TextureFormat::RGBA8
TextureFormat Textures_StringToTextureFormat(std::string& input);
