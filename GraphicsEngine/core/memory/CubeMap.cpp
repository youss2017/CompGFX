#include "CubeMap.hpp"
#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_image_resize.h>
#include <backend/VkGraphicsCard.hpp>
#include <memory/Buffer2.hpp>

/*
	There are more efficent ways of doing this but this only used in loading and its fast enough.
*/

ITexture2 GRAPHICS_API CubeMap_Create(const std::string& path, VkFormat format)
{
	int width, height, channels;
	struct Pixel {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
	uint32_t* pixels = (uint32_t*)stbi_load(path.c_str(), &width, &height, &channels, 4);
	// 1) Get Individual cube faces
	// Inverse Gamma Correction
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Pixel p = *(Pixel*)& pixels[y * width + x];
			//printf("%d %d %d before\n", r, g, b);
			p.r = (int)(pow((float)p.r / 255.0f, 2.2f) * 255.0f);
			p.g = (int)(pow((float)p.g / 255.0f, 2.2f) * 255.0f);
			p.b = (int)(pow((float)p.b / 255.0f, 2.2f) * 255.0f);
			//printf("%d %d %d after\n", r, g, b);
			auto pixel = (Pixel*)&pixels[y * width + x];
			*pixel = p;
		}
	}
	int face_size = width / 4;

	/*			
		...[T]......
		[L][F][R][B]
		...[B]......
	*/

	auto CreateFace = [width, face_size](const uint32_t* _source, const uint32_t coord_x, const uint32_t coord_y) throw() -> uint32_t* {
		uint32_t* face = new uint32_t[face_size * face_size];
		for (uint32_t sY = coord_y * face_size, fY = 0; sY < (coord_y * face_size) + face_size; sY++, fY++) {
			for (uint32_t sX = coord_x * face_size, fX = 0; sX < (coord_x * face_size) + face_size; sX++, fX++) {
				face[fY * face_size + fX] = _source[sY * width + sX];
			}
		}
		return face;
	};

	uint32_t* faces[6];
	faces[0] = CreateFace(pixels, 2, 1);
	faces[1] = CreateFace(pixels, 0, 1);
	faces[2] = CreateFace(pixels, 1, 0);
	faces[3] = CreateFace(pixels, 1, 2);
	faces[4] = CreateFace(pixels, 1, 1);
	faces[5] = CreateFace(pixels, 3, 1);
	
	// 2) Create mipmaps
	auto CreateMips = [face_size](uint32_t* face, uint32_t mipindex) throw() -> uint32_t* {
		int mipcount = std::log2(face_size);
		int size = face_size >> mipindex;
		uint32_t* mip = new uint32_t[size * size];
		stbir_resize_uint8((uint8_t*)face, face_size, face_size, face_size * sizeof(uint32_t), (uint8_t*)mip, size, size, size * 4, 4);
		return mip;
	};

	// 3) Create vulkan cubemap
	ITexture2 cubeFaces[6];
	for (int i = 0; i < 6; i++) {
		Texture2DSpecification spec;
		spec.m_Width = face_size;
		spec.m_Height = face_size;
		spec.mLayers = 1;
		spec.m_TextureUsage = TextureUsage::TEXTURE;
		spec.m_Samples = TextureSamples::MSAA_1;
		spec.m_Format = format;
		spec.m_GenerateMipMapLevels = true;
		spec.m_CreatePerFrame  = false;
		spec.m_LazilyAllocate  = false;
		spec.mUsage = 0;
		cubeFaces[i] = Texture2_Create(spec);
		Texture2_UploadPixels(cubeFaces[i], faces[i], face_size * face_size * 4);
	}

	Texture2DSpecification spec;
	spec.m_Width = face_size;
	spec.m_Height = face_size;
	spec.mLayers = 6;
	spec.m_TextureUsage = TextureUsage::TEXTURE;
	spec.m_Samples = TextureSamples::MSAA_1;
	spec.m_Format = format;
	spec.m_GenerateMipMapLevels = true;
	spec.m_CreatePerFrame = false;
	spec.m_LazilyAllocate = false;
	spec.mFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	ITexture2 cubeMap = Texture2_Create(spec);
	
	vk::VkContext context = vk::Gfx_GetContext();
	auto cmd = vk::Gfx_CreateSingleUseCmdBuffer();
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = cubeMap->m_vk_image->m_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vkCmdPipelineBarrier(cmd.cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	for (int i = 0; i < 6; i++) {
		barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = cubeFaces[i]->m_vk_image->m_image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(cmd.cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		for (int j = 0; j < std::log2(face_size); j++) {
			VkImageCopy region{};
			region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.srcSubresource.mipLevel = j;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = 1;
			region.srcOffset = {};
			region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.dstSubresource.mipLevel = j;
			region.dstSubresource.baseArrayLayer = i;
			region.dstSubresource.layerCount = 1;
			region.dstOffset = {};
			region.extent = {uint32_t(face_size >> j), uint32_t(face_size >> j), 1};
			vkCmdCopyImage(cmd.cmd, cubeFaces[i]->m_vk_image->m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cubeMap->m_vk_image->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}
	}
	barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = cubeMap->m_vk_image->m_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vkCmdPipelineBarrier(cmd.cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(cmd);

	for (int i = 0; i < 6; i++) {
		Texture2_Destroy(cubeFaces[i]);
		delete faces[i];
	}

	free(pixels);
	return cubeMap;
}
