#pragma once
#include "backend/backend_base.h"
#include <iostream>
#include <cassert>
#include <string.h>
#include <vector>
#include <vulkan/vulkan.h>

enum class BufferMemoryType
{
	// GPU_ONLY
	GPU_ONLY = 0x0, // written once, read a lot
	// CPU_TO_GPU
	CPU_TO_GPU = 0x1, // written a somtimes, read a lot
	// CPU_ONLY
	CPU_ONLY = 0x2 // written every frame
};

enum BufferType : int {
	BUFFER_TYPE_VERTEX				 = 0x0000001,
	BUFFER_TYPE_INDEX				 = 0x0000010,
	BUFFER_TYPE_UNIFORM				 = 0x0000100,
	BUFFER_TYPE_STORAGE				 = 0x0001000,
	BUFFER_TYPE_INDIRECT			 = 0x0010000,
	BUFFER_TYPE_TRANSFER_SRC         = 0x0100000,
	BUFFER_TYPE_TRANSFER_DST         = 0x1000000
};

enum class TextureFilter
{
	Nearest,
	Linear,
};

enum class TextureMIPFilter
{
	MIPMAP_Linear,
	MIPMAP_Nearest
};

enum class TextureWrapping
{
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder
};

struct TextureSamplerSpecification
{
	TextureFilter MinFilter = TextureFilter::Linear;
	TextureFilter MagFilter = TextureFilter::Linear;
	TextureMIPFilter MIPMAPFilter = TextureMIPFilter::MIPMAP_Linear;
	union
	{
		TextureWrapping WrappingMode_U = TextureWrapping::Repeat;
		TextureWrapping WrappingMode_S;
	};
	union
	{
		TextureWrapping WrappingMode_V = TextureWrapping::Repeat;
		TextureWrapping WrappingMode_T;
	};
	union
	{
		TextureWrapping WrappingMode_W = TextureWrapping::Repeat;
		TextureWrapping WrappingMode_R;
	};
	float MipMapLODBias = -1.0f; // they say this is a good value
	bool EnableAnisotropicFiltering = false;
	float MaxAnisotropicLevel = 0; // MAX 16 (on most gpus)
	// Lower end of the mipmap range to clamp access to, where 0 is the largest and most detailed mipmap level and any level higher than that is less detailed.
	float MinLOD = 0.0f;
	// Upper end of the mipmap range to clamp access to, where 0 is the largest and most detailed mipmap level and any level higher than that is less detailed.
	// This value must be greater than or equal to MinLOD. To have no upper limit on LOD set this to a large value.
	float MaxLOD = 1000.0f;
};

enum class TextureUsage
{
	TEXTURE,
	COLOR,
	DEPTH_STENCIL,
	DEPTH_ONLY
};

// Will clamp to the highest supported MSAA samples by GPU
enum class TextureSamples
{
	MSAA_1 = VK_SAMPLE_COUNT_1_BIT,
	MSAA_2 = VK_SAMPLE_COUNT_2_BIT,
	MSAA_4 = VK_SAMPLE_COUNT_4_BIT,
	MSAA_8 = VK_SAMPLE_COUNT_8_BIT,
	MSAA_16 = VK_SAMPLE_COUNT_16_BIT,
	MSAA_32 = VK_SAMPLE_COUNT_32_BIT,
	MSAA_64 = VK_SAMPLE_COUNT_64_BIT
};

struct Texture2DSpecification
{
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t mLayers = 1;
	TextureUsage m_TextureUsage = TextureUsage::TEXTURE;
	TextureSamples m_Samples = TextureSamples::MSAA_1;
	VkFormat m_Format;
	VkImageCreateFlags mFlags = 0;
	VkImageUsageFlags mUsage = 0;
	VkImageTiling mTiling = VK_IMAGE_TILING_OPTIMAL;
	bool m_GenerateMipMapLevels = false;
	bool m_CreatePerFrame = false;
	bool m_LazilyAllocate = false;
	bool mCreateViewPerMip = false;
	VkComponentMapping mTextureSwizzling = {};
};
