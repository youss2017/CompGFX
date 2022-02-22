#include "Texture2.hpp"
#include <backend/VkGraphicsCard.hpp>
#include "Buffer2.hpp"
#include <stb/stb_image.h>

ITexture2 Texture2_Create(GraphicsContext context, const Texture2DSpecification& specification)
{
	using namespace VkAlloc;
	assert(!specification.m_CreatePerFrame);
	IMAGE_DESCRIPITION desc;
	desc.m_properties = DEVICE_MEMORY_PROPERTY::GPU_ONLY;
	desc.m_flags = 0;
	desc.m_imageType = VK_IMAGE_TYPE_2D;
	desc.m_format = (VkFormat)specification.m_Format;
	desc.m_extent.width = specification.m_Width;
	desc.m_extent.height = specification.m_Height;
	desc.m_extent.depth = 1;
	desc.m_mipLevels = specification.m_GenerateMipMapLevels ? std::floor(std::log2(std::max(specification.m_Width, specification.m_Height))) + 1 : 1;
	desc.m_arrayLayers = 1;
	desc.m_samples = (VkSampleCountFlagBits)specification.m_Samples;;
	desc.m_tiling = VK_IMAGE_TILING_OPTIMAL;
	switch (specification.m_TextureUsage)
	{
		case TextureUsage::TEXTURE:
			desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;
		case TextureUsage::COLOR_ATTACHMENT:
			desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			break;
		case TextureUsage::DEPTH_ATTACHMENT:
			desc.m_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			break;
		default:
			desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			assert(0);
	}
	ITexture2 texture = new GPUTexture2D_2();
	texture->m_vk_description = desc;
	texture->m_context = context;
	texture->m_specification = specification;
	VkAlloc::CreateImages(ToVKContext(context)->m_future_memory_context, 1, &desc, &texture->m_vk_image);
	VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = (VkFormat)specification.m_Format;
	auto GetVkSwizzle = [](TextureSwizzle swizz) throw()->VkComponentSwizzle
	{
		if (swizz == TextureSwizzle::SwizzleRED)
			return VK_COMPONENT_SWIZZLE_R;
		if (swizz == TextureSwizzle::SwizzleGREEN)
			return VK_COMPONENT_SWIZZLE_G;
		if (swizz == TextureSwizzle::SwizzleBLUE)
			return VK_COMPONENT_SWIZZLE_B;
		if (swizz == TextureSwizzle::SwizzleALPHA)
			return VK_COMPONENT_SWIZZLE_A;
		if (swizz == TextureSwizzle::SwizzleONE)
			return VK_COMPONENT_SWIZZLE_ONE;
		if (swizz == TextureSwizzle::SwizzleZERO)
			return VK_COMPONENT_SWIZZLE_ZERO;
		return VK_COMPONENT_SWIZZLE_IDENTITY;
	};
	viewInfo.image = texture->m_vk_image->m_image;
	viewInfo.components.r = GetVkSwizzle(specification.m_Sampler.SwizzleRed);
	viewInfo.components.g = GetVkSwizzle(specification.m_Sampler.SwizzleGreen);
	viewInfo.components.b = GetVkSwizzle(specification.m_Sampler.SwizzleBlue);
	viewInfo.components.a = GetVkSwizzle(specification.m_Sampler.SwizzleAlpha);
	viewInfo.subresourceRange.aspectMask = specification.m_TextureUsage == TextureUsage::DEPTH_ATTACHMENT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = desc.m_mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	texture->m_vk_aspectmask = viewInfo.subresourceRange.aspectMask;
	vkcheck(vkCreateImageView(ToVKContext(context)->defaultDevice, &viewInfo, nullptr, &texture->m_vk_view));

	return texture;
}

void Texture2_UploadPixels(ITexture2 texture, void* pixels, uint32_t size)
{
	vk::VkContext context = (vk::VkContext)texture->m_context;
	// create staging buffer
	IBuffer2 staging_buffer = Buffer2_Create(texture->m_context, BufferType::INTE_TRANSFER_SRC, size, BufferMemoryType::CPU_ONLY);
	// load data to buffer
	char8_t* staging_buffer_ptr = Buffer2_Map(staging_buffer);
	memcpy(staging_buffer_ptr, pixels, size);
	Buffer2_Flush(staging_buffer, 0, size);
	Buffer2_Unmap(staging_buffer);
	// Create cmd_buffer and pool
	VkCommandPool pool = vk::Gfx_CreateCommandPool(context, true, false);
	VkCommandBuffer cmd_buffer = vk::Gfx_AllocCommandBuffer(context, pool, true);
	VkFence fence = vk::Gfx_CreateFence(context);
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
	barrier.subresourceRange.levelCount = (texture->m_specification.m_GenerateMipMapLevels) ? (std::floor(std::log2(std::max(texture->m_specification.m_Width, texture->m_specification.m_Height))) + 1) : 1;;
	barrier.subresourceRange.baseArrayLayer = 0;
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
	vkCmdCopyBufferToImage(cmd_buffer, staging_buffer->m_vk_buffer->m_buffer, texture->m_vk_image->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copy_region);

	// transfer image back to readable format
	VkImageMemoryBarrier barrier_readable = barrier;
	barrier_readable.srcAccessMask = CurrentAccessFlag;
	barrier_readable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier_readable.oldLayout = texture->m_vk_current_layout;
	barrier_readable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier_readable.subresourceRange.levelCount = (texture->m_specification.m_GenerateMipMapLevels) ? (std::floor(std::log2(std::max(texture->m_specification.m_Width, texture->m_specification.m_Height))) + 1) : 1;
	texture->m_vk_current_layout = barrier_readable.newLayout;
	CurrentAccessFlag = barrier_readable.dstAccessMask;
	vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &barrier_readable);
	CurrentAccessFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	vkEndCommandBuffer(cmd_buffer);
	vk::Gfx_SubmitCmdBuffers(context->defaultQueue, { cmd_buffer }, {}, {}, {}, fence);
	vkWaitForFences(context->defaultDevice, 1, &fence, true, UINT64_MAX);
	// cleanup
	vkDestroyCommandPool(context->defaultDevice, pool, context->m_allocation_callback);
	vkDestroyFence(context->defaultDevice, fence, context->m_allocation_callback);
	Buffer2_Destroy(staging_buffer);

	if (texture->m_specification.m_GenerateMipMapLevels)
		Texture2_UpdateMipmaps(texture);
}

void Texture2_UpdateMipmaps(ITexture2 texture)
{
	using namespace vk;

	uint32_t miplevels = std::floor(std::log2(std::max(texture->m_specification.m_Width, texture->m_specification.m_Height))) + 1;
	VkContext context = (VkContext)texture->m_context;
	VkCommandPool pool = Gfx_CreateCommandPool(context, true, false);
	VkCommandBuffer cmd = Gfx_AllocCommandBuffer(context, pool, true);
	VkFence fence = Gfx_CreateFence(context);

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

	vkDestroyFence(context->defaultDevice, fence, context->m_allocation_callback);
	vkDestroyCommandPool(context->defaultDevice, pool, context->m_allocation_callback);
}

void Texture2_Destroy(ITexture2 texture)
{
	VkAlloc::DestroyImages(ToVKContext(texture->m_context)->m_future_memory_context, 1, &texture->m_vk_image);
	VkDevice device = ToVKContext(texture->m_context)->defaultDevice;
	for (int i = 0; i < texture->m_vk_views.size(); i++)
		vkDestroyImageView(device, texture->m_vk_views[i], nullptr);
	if(texture->m_vk_view)
		vkDestroyImageView(device, texture->m_vk_view, nullptr);
	delete texture;
}

ITexture2 Texture2_CreateFromFile(GraphicsContext context, const char* path, bool mipmaps)
{
	int w, h, c;
	void* pixels = stbi_load(path, &w, &h, &c, 4);
	if (!pixels) {
		logerror("Could not load texture from file!");
		return nullptr;
	}
	Texture2DSpecification specification;
	specification.m_Width = w;
	specification.m_Height = h;
	specification.m_Format = TextureFormat::RGBA8;
	specification.m_GenerateMipMapLevels = mipmaps;
	ITexture2 texture = Texture2_Create(context, specification);
	Texture2_UploadPixels(texture, pixels, w * h * sizeof(uint32_t));
	stbi_image_free(pixels);
	return texture;
}
