#include "Texture2.hpp"
#include <backend/VkGraphicsCard.hpp>
#include "Buffer2.hpp"
#include <stb_image.h>
#include <StringUtils.hpp>

ITexture2 GRAPHICS_API Texture2_Create(const Texture2DSpecification& specification)
{
	using namespace VkAlloc;
	vk::VkContext context = vk::Gfx_GetContext();
	IMAGE_DESCRIPITION desc{};
	desc.m_properties = DEVICE_MEMORY_PROPERTY::GPU_ONLY;
	desc.m_flags = specification.mFlags;
	desc.m_imageType = VK_IMAGE_TYPE_2D;
	desc.m_format = (VkFormat)specification.m_Format;
	desc.m_extent.width = specification.m_Width;
	desc.m_extent.height = specification.m_Height;
	desc.m_extent.depth = 1;
	desc.m_mipLevels = specification.m_GenerateMipMapLevels ? std::min(std::log2(specification.m_Width), std::log2(specification.m_Height)) : 1;
	desc.m_arrayLayers = specification.mLayers;;
	desc.m_samples = (VkSampleCountFlagBits)specification.m_Samples;
	desc.m_usage = specification.mUsage;
	desc.m_tiling = specification.mTiling;
	switch (specification.m_TextureUsage)
	{
		case TextureUsage::TEXTURE:
			desc.m_usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;
		case TextureUsage::COLOR:
			desc.m_usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;
		case TextureUsage::DEPTH_STENCIL:
		case TextureUsage::DEPTH_ONLY:
			desc.m_usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;
		default:
			desc.m_usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			assert(0);
	}
	ITexture2 texture = new GPUTexture2D_2();
	texture->m_vk_description = desc;
	texture->m_context = context;
	texture->m_specification = specification;
	texture->mMipCount = desc.m_mipLevels;
	desc.m_lazyAllocate = specification.m_LazilyAllocate;
	VkAlloc::CreateImages(context->m_future_memory_context, 1, &desc, &texture->m_vk_image);
	std::string userData = "IMAGE";
	vmaSetAllocationUserData(context->m_future_memory_context->m_allocator, texture->m_vk_image->m_suballocation.m_allocation, userData.data());
	VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.viewType = (desc.m_flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = (VkFormat)specification.m_Format;
	viewInfo.image = texture->m_vk_image->m_image;
	viewInfo.components = specification.mTextureSwizzling;
	if (specification.m_TextureUsage == TextureUsage::COLOR || specification.m_TextureUsage == TextureUsage::TEXTURE)
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	else if(specification.m_TextureUsage == TextureUsage::DEPTH_STENCIL)
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	else
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	texture->m_vk_aspectmask = viewInfo.subresourceRange.aspectMask;
	vkcheck(vkCreateImageView(context->defaultDevice, &viewInfo, nullptr, &texture->m_vk_view));
	vk::VkContext cont = context;
	if (specification.m_CreatePerFrame)
	{
		texture->m_vk_views_per_frame.push_back(texture->m_vk_view);
		texture->m_vk_images_per_frame.push_back(texture->m_vk_image->m_image);
		VkImageCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.imageType = desc.m_imageType;
		createInfo.format = desc.m_format;
		createInfo.extent = desc.m_extent;
		createInfo.mipLevels = desc.m_mipLevels;
		createInfo.arrayLayers = desc.m_arrayLayers;
		createInfo.samples = desc.m_samples;
		createInfo.tiling = desc.m_tiling;
		createInfo.usage = desc.m_usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImage image;
		for (unsigned int i = 1; i < gFrameOverlapCount; i++) {
			vkCreateImage(cont->defaultDevice, &createInfo, nullptr, &image);
			auto& suballoc = texture->m_vk_image->m_suballocation;
			vkBindImageMemory(cont->defaultDevice, image, suballoc.m_allocation_info.deviceMemory, suballoc.m_allocation_info.offset);
			texture->m_vk_images_per_frame.push_back(image);
			viewInfo.image = image;
			VkImageView view;
			vkCreateImageView(cont->defaultDevice, &viewInfo, nullptr, &view);
			texture->m_vk_views_per_frame.push_back(view);
		}
	}
	else {
		for (int i = 0; i < gFrameOverlapCount; i++) {
			texture->m_vk_views_per_frame.push_back(texture->m_vk_view);
			texture->m_vk_images_per_frame.push_back(texture->m_vk_image->m_image);
		}
	}

	if (specification.mCreateViewPerMip) {
		assert(specification.m_CreatePerFrame);
		for (uint32_t i = 0; i < gFrameOverlapCount; i++) {
			std::vector<VkImageView> views;
			for (uint32_t i = 0; i < desc.m_mipLevels; i++) {
				viewInfo.subresourceRange.baseMipLevel = i;
				viewInfo.subresourceRange.levelCount = 1;
				VkImageView view;
				vkCreateImageView(cont->defaultDevice, &viewInfo, nullptr, &view);
				views.push_back(view);
			}
			texture->mMipmapViews.mMipmapViewsPerFrame.push_back(views);
		}
	}

	return texture;
}

void GRAPHICS_API Texture2_UploadPixels(ITexture2 texture, void* pixels, uint32_t size)
{
	vk::VkContext context = (vk::VkContext)texture->m_context;
	// create staging buffer
	IBuffer2 staging_buffer = Buffer2_Create(BUFFER_TYPE_TRANSFER_SRC, size, BufferMemoryType::CPU_ONLY, false, false);
	// load data to buffer
	void* staging_buffer_ptr = Buffer2_Map(staging_buffer);
	memcpy(staging_buffer_ptr, pixels, size);
	Buffer2_Flush(staging_buffer, 0, size);
	Buffer2_Unmap(staging_buffer);
	// Create cmd_buffer and pool
	VkCommandPool pool = vk::Gfx_CreateCommandPool(context, true, false);
	VkCommandBuffer cmd_buffer = vk::Gfx_AllocCommandBuffer(pool, true);
	VkFence fence = vk::Gfx_CreateFence(false);
	// transition image into suitable layout
	VkAccessFlags CurrentAccessFlag = 0;
	VkPipelineStageFlags CurrentPipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = CurrentAccessFlag;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = texture->m_vk_current_layout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	texture->m_vk_current_layout = barrier.newLayout;
	CurrentAccessFlag = barrier.dstAccessMask;

	barrier.image = texture->m_vk_image->m_image;
	barrier.subresourceRange.aspectMask = texture->m_vk_aspectmask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.layerCount = 1;
	vk::Gfx_StartCommandBuffer(cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkCmdPipelineBarrier(cmd_buffer,
		CurrentPipelineStage,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, 0, 0, 0, 1, &barrier);
	CurrentPipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	// copy data from staging buffer to image
	VkBufferImageCopy buffer_copy_region{};
	buffer_copy_region.imageSubresource.aspectMask = barrier.subresourceRange.aspectMask;
	buffer_copy_region.imageSubresource.mipLevel = 0;
	buffer_copy_region.imageSubresource.baseArrayLayer = 0;
	buffer_copy_region.imageSubresource.layerCount = 1;
	buffer_copy_region.imageExtent.width = texture->m_specification.m_Width;
	buffer_copy_region.imageExtent.height = texture->m_specification.m_Height;
	buffer_copy_region.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(cmd_buffer, staging_buffer->mBuffers[0], texture->m_vk_image->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copy_region);

	// transfer image back to readable format
	VkImageMemoryBarrier barrier_readable = barrier;
	barrier_readable.srcAccessMask = CurrentAccessFlag;
	barrier_readable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier_readable.oldLayout = texture->m_vk_current_layout;
	barrier_readable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier_readable.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier_readable.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	texture->m_vk_current_layout = barrier_readable.newLayout;
	CurrentAccessFlag = barrier_readable.dstAccessMask;
	vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &barrier_readable);
	CurrentAccessFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	vkEndCommandBuffer(cmd_buffer);
	vk::Gfx_SubmitCmdBuffers(context->defaultQueue, { cmd_buffer }, {}, {}, {}, fence);
	vkWaitForFences(context->defaultDevice, 1, &fence, true, UINT64_MAX);
	// cleanup
	vkDestroyCommandPool(context->defaultDevice, pool, 0);
	vkDestroyFence(context->defaultDevice, fence, 0);
	Buffer2_Destroy(staging_buffer);

	if (texture->m_specification.m_GenerateMipMapLevels)
		Texture2_UpdateMipmaps(texture);
}

void GRAPHICS_API Texture2_UpdateMipmaps(ITexture2 texture)
{
	using namespace vk;

	uint32_t miplevels = std::floor(std::log2(std::min(texture->m_specification.m_Width, texture->m_specification.m_Height)));
	VkContext context = (VkContext)texture->m_context;
	VkCommandPool pool = Gfx_CreateCommandPool(context, true, false);
	VkCommandBuffer cmd = Gfx_AllocCommandBuffer(pool, true);
	VkFence fence = Gfx_CreateFence(false);

	vk::Gfx_StartCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageMemoryBarrier MIP0Barrier0{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	MIP0Barrier0.srcAccessMask = 0;
	MIP0Barrier0.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	MIP0Barrier0.oldLayout = texture->m_vk_current_layout;
	MIP0Barrier0.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	MIP0Barrier0.srcQueueFamilyIndex = MIP0Barrier0.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	MIP0Barrier0.image = texture->m_vk_image->m_image;
	MIP0Barrier0.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	MIP0Barrier0.subresourceRange.layerCount = 1;
	MIP0Barrier0.subresourceRange.baseMipLevel = 0;
	MIP0Barrier0.subresourceRange.levelCount = 1;

	VkImageMemoryBarrier MIPImageBarrier0{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	MIPImageBarrier0.srcAccessMask = 0;
	MIPImageBarrier0.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	MIPImageBarrier0.oldLayout = texture->m_vk_current_layout_mipmap;
	MIPImageBarrier0.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	MIPImageBarrier0.srcQueueFamilyIndex = MIPImageBarrier0.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	MIPImageBarrier0.image = texture->m_vk_image->m_image;
	MIPImageBarrier0.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	MIPImageBarrier0.subresourceRange.layerCount = 1;
	MIPImageBarrier0.subresourceRange.baseMipLevel = 1;
	MIPImageBarrier0.subresourceRange.levelCount = miplevels - 1;
	texture->m_vk_current_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	texture->m_vk_current_layout_mipmap = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	VkImageMemoryBarrier Barriers[2] = { MIP0Barrier0, MIPImageBarrier0 };

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 2, Barriers);

	for (uint32_t i = 1; i < miplevels; i++)
	{
		int32_t SourceWidth = texture->m_specification.m_Width;
		int32_t SourceHeight = texture->m_specification.m_Height;
		VkImageBlit blit{};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.layerCount = 1;
		blit.srcSubresource.mipLevel = 0;
		blit.dstSubresource = blit.srcSubresource;
		blit.dstSubresource.mipLevel = i;
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { SourceWidth, SourceHeight, 1 };
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { std::max(SourceWidth >> i, 1), std::max(SourceHeight >> i, 1), 1 };
		vkCmdBlitImage(cmd, texture->m_vk_image->m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->m_vk_image->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
	}

	VkImageMemoryBarrier MIP0Barrier1{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	MIP0Barrier1.srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	MIP0Barrier1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	MIP0Barrier1.dstAccessMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	MIP0Barrier1.oldLayout = texture->m_vk_current_layout;
	MIP0Barrier1.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	MIP0Barrier1.srcQueueFamilyIndex = MIP0Barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	MIP0Barrier1.image = texture->m_vk_image->m_image;
	MIP0Barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	MIP0Barrier1.subresourceRange.layerCount = 1;
	MIP0Barrier1.subresourceRange.baseMipLevel = 0;
	MIP0Barrier1.subresourceRange.levelCount = 1;

	VkImageMemoryBarrier MIPImageBarrier1{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	MIPImageBarrier1.srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	MIPImageBarrier1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	MIPImageBarrier1.oldLayout = texture->m_vk_current_layout_mipmap;
	MIPImageBarrier1.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	MIPImageBarrier1.srcQueueFamilyIndex = MIPImageBarrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	MIPImageBarrier1.image = texture->m_vk_image->m_image;
	MIPImageBarrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	MIPImageBarrier1.subresourceRange.layerCount = 1;
	MIPImageBarrier1.subresourceRange.baseMipLevel = 1;
	MIPImageBarrier1.subresourceRange.levelCount = miplevels - 1;
	texture->m_vk_current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texture->m_vk_current_layout_mipmap = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkImageMemoryBarrier Barriers2[2] = { MIP0Barrier1 , MIPImageBarrier1 };

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, Barriers2);

	vkEndCommandBuffer(cmd);
	vk::Gfx_SubmitCmdBuffers(context->defaultQueue, { cmd }, {}, {}, {}, fence);
	vkWaitForFences(context->defaultDevice, 1, &fence, true, 5000'0000'000);

	vkDestroyFence(context->defaultDevice, fence, 0);
	vkDestroyCommandPool(context->defaultDevice, pool, 0);
}

void GRAPHICS_API Texture2_ReadPixels(ITexture2 texture, VkAccessFlagBits finalAccessFlags, VkImageLayout currentLayout, uint32_t pixelSize, void* buffer) {
	auto cmd = vk::Gfx_CreateSingleUseCmdBuffer();
	vk::Framebuffer_TransistionImage(cmd.cmd, texture, texture->m_vk_aspectmask, 0, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, currentLayout);
	auto spec = texture->m_specification;
	size_t dstBufferSize = spec.m_Width * spec.m_Height * pixelSize;
	void* dstBuffer = Gmalloc(dstBufferSize, BufferType::BUFFER_TYPE_TRANSFER_DST, false);
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.imageSubresource.aspectMask = texture->m_vk_aspectmask;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = { spec.m_Width, spec.m_Height, 1 };
	vkCmdCopyImageToBuffer(cmd.cmd, texture->m_vk_image->m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Gbuffer(dstBuffer)->mBuffers[0], 1, &region);
	vk::Framebuffer_TransistionImage(cmd.cmd, texture, texture->m_vk_aspectmask, 0, finalAccessFlags, currentLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(cmd);
	GverifyRead(dstBuffer);
	memcpy(buffer, dstBuffer, dstBufferSize);
	Gfree(dstBuffer);
}

void GRAPHICS_API Texture2_Destroy(ITexture2 texture)
{
	VkAlloc::DestroyImages((texture->m_context)->m_future_memory_context, 1, &texture->m_vk_image);
	VkDevice device = (texture->m_context)->defaultDevice;
	if (texture->m_specification.m_CreatePerFrame) {
		vkDestroyImageView(device, texture->m_vk_views_per_frame[0], nullptr);
		for (int i = 1; i < texture->m_vk_views_per_frame.size(); i++) {
			vkDestroyImage(device, texture->m_vk_images_per_frame[i], nullptr);
			vkDestroyImageView(device, texture->m_vk_views_per_frame[i], nullptr);
		}
	}
	else	
		vkDestroyImageView(device, texture->m_vk_view, nullptr);
	for (auto& list : texture->mMipmapViews.mMipmapViewsPerFrame) {
		for (auto& view : list) {
			vkDestroyImageView(device, view, nullptr);
		}
	}
	delete texture;
}

ITexture2 GRAPHICS_API Texture2_CreateFromFile(const char* path, bool mipmaps)
{
	int w, h, c;
	unsigned int* pixels = (unsigned int*)stbi_load(path, &w, &h, &c, 4);
	if (!pixels) {
		logerror("Could not load texture from file!");
		return nullptr;
	}
	struct Pixel {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			Pixel p = *(Pixel*)&pixels[y * w + x];
			//printf("%d %d %d before\n", r, g, b);
			p.r = (int)(pow((float)p.r / 255.0f, 2.2f) * 255.0f);
			p.g = (int)(pow((float)p.g / 255.0f, 2.2f) * 255.0f);
			p.b = (int)(pow((float)p.b / 255.0f, 2.2f) * 255.0f);
			//printf("%d %d %d after\n", r, g, b);
			auto pixel = (Pixel*)&pixels[y * w + x];
			*pixel = p;
		}
	}
	Texture2DSpecification specification;
	specification.m_Width = w;
	specification.m_Height = h;
	specification.m_Format = VK_FORMAT_R8G8B8A8_UNORM;
	specification.m_GenerateMipMapLevels = mipmaps;
	specification.m_TextureUsage = TextureUsage::TEXTURE;
	ITexture2 texture = Texture2_Create(specification);
	Texture2_UploadPixels(texture, pixels, w * h * sizeof(uint32_t));
	stbi_image_free(pixels);
	return texture;
}

void GRAPHICS_API Texture2_RemoveAlphaChannel(uint32_t* srcPixels, char* dstPixels, int width, int height) {
	struct Pixel24 {
		char r;
		char g;
		char b;
	};
	Pixel24* dst = (Pixel24*)dstPixels;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			dst[y * width + x].r = (srcPixels[y * width + x] >> 16) & 0xff;
			dst[y * width + x].g = (srcPixels[y * width + x] >> 8) & 0xff;
			dst[y * width + x].b = (srcPixels[y * width + x] >> 0) & 0xff;
		}
	}
}
