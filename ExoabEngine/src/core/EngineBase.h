#pragma once
#include "backend/backend_base.h"
#include "../utils/common.hpp"
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
	CPU_TO_CPU = 0x1, // written a somtimes, read a lot
	// CPU_ONLY
	CPU_ONLY = 0x2 // written every frame
};

enum class EngineAPIType
{
	Vulkan = 0x0,
	OpenGL = 0x1
};

typedef enum BufferType {
	VertexBuffer      = 0x0000001,
	IndexBuffer       = 0x0000010,
	UniformBuffer     = 0x0000100,
	StorageBuffer     = 0x0001000,
	IndirectBuffer    = 0x0010000,
	INTE_TRANSFER_SRC = 0x0100000,
	INTE_TRANSFER_DST = 0x1000000
} BufferType;

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

enum class TextureSwizzle
{
	Identity = 0,
	SwizzleONE,
	SwizzleZERO,
	SwizzleRED,
	SwizzleGREEN,
	SwizzleBLUE,
	SwizzleALPHA,
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
	TextureSwizzle SwizzleRed = TextureSwizzle::Identity;
	TextureSwizzle SwizzleGreen = TextureSwizzle::Identity;
	TextureSwizzle SwizzleBlue = TextureSwizzle::Identity;
	TextureSwizzle SwizzleAlpha = TextureSwizzle::Identity;
	float MipMapLODBias = -1.0f; // they say this is a good value
	bool EnableAnisotropicFiltering = false;
	float MaxAnisotropicLevel = 0; // MAX 16 (on most gpus)
	// Lower end of the mipmap range to clamp access to, where 0 is the largest and most detailed mipmap level and any level higher than that is less detailed.
	float MinLOD = 0.0f;
	// Upper end of the mipmap range to clamp access to, where 0 is the largest and most detailed mipmap level and any level higher than that is less detailed.
	// This value must be greater than or equal to MinLOD. To have no upper limit on LOD set this to a large value.
	float MaxLOD = 1000.0f;
};

// TODO: Maybe add support for texture compression formats (not really needed though since were propabaly not going to use more than 150mb vram for textures)
enum class TextureFormat
{
	UNDEFINED = VK_FORMAT_UNDEFINED,
	R8 = VK_FORMAT_R8_UNORM,
	R8_SNORM = VK_FORMAT_R8_SNORM,
	R16 = VK_FORMAT_R16_UNORM,
	R16_SNORM = VK_FORMAT_R16_SNORM,
	RG8 = VK_FORMAT_R8G8_UNORM,
	RG8_SNORM = VK_FORMAT_R8G8_SNORM,
	RG16 = VK_FORMAT_R16G16_UNORM,
	RG16_SNORM = VK_FORMAT_R16G16_SNORM,
	RGB8 = VK_FORMAT_R8G8B8_UNORM,
	RGB8_SNORM = VK_FORMAT_R8G8B8_SNORM,
	RGB16_SNORM = VK_FORMAT_R16G16B16_SNORM,
	RGBA8 = VK_FORMAT_R8G8B8A8_UNORM,
	RGBA8_SNORM = VK_FORMAT_R8G8B8A8_SNORM,
	RGBA16 = VK_FORMAT_R16G16B16A16_UNORM,
	SRGB8 = VK_FORMAT_R8G8B8_SRGB,
	SRGB8_ALPHA8 = VK_FORMAT_R8G8B8A8_SRGB,
	R16F = VK_FORMAT_R16_SFLOAT,
	RG16F = VK_FORMAT_R16G16_SFLOAT,
	RGB16F = VK_FORMAT_R16G16B16_SFLOAT,
	RGBA16F = VK_FORMAT_R16G16B16A16_SFLOAT,
	R32F = VK_FORMAT_R32_SFLOAT,
	D32F = VK_FORMAT_D32_SFLOAT,
	RG32F = VK_FORMAT_R32G32_SFLOAT,
	RGB32F = VK_FORMAT_R32G32B32_SFLOAT,
	RGBA32F = VK_FORMAT_R32G32B32A32_SFLOAT,
	R8I = VK_FORMAT_R8_SINT,
	R8UI = VK_FORMAT_R8_UINT,
	R16I = VK_FORMAT_R16_SINT,
	R16UI = VK_FORMAT_R16_UINT,
	R32I = VK_FORMAT_R32_SINT,
	R32UI = VK_FORMAT_R32_UINT,
	RG8I = VK_FORMAT_R8G8B8_SINT,
	RG8UI = VK_FORMAT_R8G8B8_UINT,
	RG16I = VK_FORMAT_R16G16_SINT,
	RG16UI = VK_FORMAT_R16G16_UINT,
	RG32I = VK_FORMAT_R32G32_SINT,
	RG32UI = VK_FORMAT_R32G32_UINT,
	RGB8I = VK_FORMAT_R8G8B8_SINT,
	RGB8UI = VK_FORMAT_R8G8B8_UINT,
	RGB16I = VK_FORMAT_R16G16B16_SINT,
	RGB16UI = VK_FORMAT_R16G16B16_UINT,
	RGB32I = VK_FORMAT_R32G32B32_SINT,
	RGB32UI = VK_FORMAT_R32G32B32_UINT,
	RGBA8I = VK_FORMAT_R8G8B8A8_SINT,
	RGBA8UI = VK_FORMAT_R8G8B8A8_UINT,
	RGBA16I = VK_FORMAT_R16G16B16A16_SINT,
	RGBA16UI = VK_FORMAT_R16G16B16A16_UINT,
	RGBA32I = VK_FORMAT_R32G32B32A32_SINT,
	RGBA32UI = VK_FORMAT_R32G32B32A32_UINT,
	BGRA8 = VK_FORMAT_B8G8R8A8_UNORM,
	SBGR8_ALPHA8 = VK_FORMAT_B8G8R8_SRGB,
	R11G11B10F = VK_FORMAT_B10G11R11_UFLOAT_PACK32, // It is R11G11B10 in vulkan, (checked in renderdoc) doesn't matter anyway
};

enum class TextureUsage
{
	DEFAULT,
	TEXTURE = DEFAULT,
	COLOR_ATTACHMENT,
	DEPTH_ATTACHMENT,
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
	TextureUsage m_TextureUsage = TextureUsage::DEFAULT;
	TextureSamples m_Samples = TextureSamples::MSAA_1;
	TextureFormat m_Format;
	bool m_GenerateMipMapLevels;
	bool m_CreatePerFrame = false;
	bool m_LazilyAllocate = false;
};

namespace vk
{
	uint32_t AlignSizeToUniformAlignment(GraphicsContext context, uint32_t size);
}