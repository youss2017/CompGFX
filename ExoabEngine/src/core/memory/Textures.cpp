#include "Textures.hpp"
#include "Buffer2.hpp"
#include "../backend/VkGraphicsCard.hpp"
#include "../backend/GLGraphicsCard.hpp"
#include "../../utils/StringUtils.hpp"
#include <stb_image.h>
#include "vulkan_memory.h"
#include <cassert>
#include <cmath>

PFN_GPUTexture2D_Create *GPUTexture2D_Create = nullptr;
PFN_GPUTexture2D_UploadPixels *GPUTexture2D_UploadPixels = nullptr;
PFN_GPUTexture2D_GenerateMips *GPUTexture2D_GenerateMips = nullptr;
PFN_GPUTexture2D_GetImage *GPUTexture2D_GetImage = nullptr;
PFN_GPUTexture2D_GetView *GPUTexture2D_GetView = nullptr;
PFN_GPUTexture2D_Destroy *GPUTexture2D_Destroy = nullptr;

/* ********************************************* ENGINE TEXTURE 2D ********************************************* */
static void GetOpenGLTextureFormatType(TextureFormat format, GLenum &OutInternalFormat, GLenum &OutFormat, GLenum &OutDataType);

IGPUTexture2D Vulkan_GPUTexture2D_Create(GraphicsContext _context, Texture2DSpecification *_specification)
{
	Texture2DSpecification specification = *_specification;
	GPUTexture2D *result = new GPUTexture2D();
	result->m_Context = _context;
	result->m_specification = specification;
	result->m_Vk_AllocatedMipMaps = specification.m_GenerateMipMapLevels;
	result->m_MipLevels = specification.m_GenerateMipMapLevels ? std::floor(std::log2(std::max(specification.m_Width, specification.m_Height))) + 1 : 1;
	result->m_CreatePerFrame = specification.m_CreatePerFrame;
	if (specification.m_Samples != TextureSamples::MSAA_1)
	{
		if (specification.m_GenerateMipMapLevels)
		{
			logwarning("Cannot use mipmaps on multisampled textures!");
			specification.m_GenerateMipMapLevels = false;
			result->m_Vk_AllocatedMipMaps = false;
		}
	}
	if (specification.m_TextureUsage == TextureUsage::COLOR_ATTACHMENT && specification.m_GenerateMipMapLevels)
		logwarning("Creating Tex2D Color Attachment with MIPMAPs");

	assert(_context && specification.m_Width > 0 && specification.m_Height > 0);
	result->m_ApiType = 0;
	vk::VkContext context = (vk::VkContext)result->m_Context;
	if (context->m_MaxMSAASamples < specification.m_Samples)
	{
		logwarning("Requested MSAA Count is not available, using a supported MSAA sample count instead.");
		specification.m_Samples = context->m_MaxMSAASamples;
	}

	VkImageCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = (VkFormat)specification.m_Format;
	createInfo.extent.width = specification.m_Width;
	createInfo.extent.height = specification.m_Height;
	createInfo.extent.depth = 1;
	if (!specification.m_GenerateMipMapLevels)
		createInfo.mipLevels = 1;
	else
	{
		createInfo.mipLevels = std::floor(std::log2(std::max(specification.m_Width, specification.m_Height))) + 1;
	}
	createInfo.arrayLayers = 1;
	createInfo.samples = (VkSampleCountFlagBits)specification.m_Samples;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	if (result->m_specification.m_TextureUsage == TextureUsage::TEXTURE)
		createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	else if (result->m_specification.m_TextureUsage == TextureUsage::COLOR_ATTACHMENT)
		createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	else if (result->m_specification.m_TextureUsage == TextureUsage::DEPTH_ATTACHMENT)
		createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	else
		assert(0 && "Invalid Texture View!");
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (specification.m_LazilyAllocate)
	{
		// if (specification.m_TextureUsage == TextureUsage::COLOR_ATTACHMENT)
		//	createInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		// else if(specification.m_TextureUsage == TextureUsage::DEPTH_ATTACHMENT)
		//	createInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		// else
		//	log_warning("Cannot Lazily Allocate texture that is not color/depth attachment.");
	}

	VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
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
	viewInfo.components.r = GetVkSwizzle(specification.m_Sampler.SwizzleRed);
	viewInfo.components.g = GetVkSwizzle(specification.m_Sampler.SwizzleGreen);
	viewInfo.components.b = GetVkSwizzle(specification.m_Sampler.SwizzleBlue);
	viewInfo.components.a = GetVkSwizzle(specification.m_Sampler.SwizzleAlpha);

	if (specification.m_TextureUsage == TextureUsage::TEXTURE || specification.m_TextureUsage == TextureUsage::COLOR_ATTACHMENT)
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	else if (specification.m_TextureUsage == TextureUsage::DEPTH_ATTACHMENT)
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	else
		assert(0 && "Error! internal");
	result->m_ImageAspectFlags = viewInfo.subresourceRange.aspectMask;

	if (!specification.m_LazilyAllocate)
	{
		vulkan_memory_allocate_texture2d((VulkanMemoryContext)context->m_memory_context, &createInfo,
										 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										 (VkImage *)&result->m_NativeHandle, &result->m_memory, &result->m_MemoryOffset, &result->m_AllocationIndex);
	}
	else
	{
		// TODO: Actually Lazially Allocate
		vulkan_memory_allocate_texture2d((VulkanMemoryContext)context->m_memory_context, &createInfo,
										 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										 (VkImage *)&result->m_NativeHandle, &result->m_memory, &result->m_MemoryOffset, &result->m_AllocationIndex);
		// vulkan_memory_allocate_texture2d((VulkanMemoryContext)context->m_memory_context, &createInfo,
		//								 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		//								 (VkImage *)&result->m_NativeHandle, &result->m_memory, &result->m_MemoryOffset, &result->m_AllocationIndex);
	}
	viewInfo.image = (VkImage)result->m_NativeHandle;

	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = result->m_MipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	vkCreateImageView(context->defaultDevice, &viewInfo, context->m_allocation_callback, &result->view);

	if (specification.m_CreatePerFrame)
	{
		int FrameCount = context->FrameInfo->m_FrameCount;
		std::vector<VkImage> images;
		std::vector<VkImageView> views;
		images.push_back(viewInfo.image);
		views.push_back(result->view);
		for (int i = 1; i < FrameCount; i++)
		{
			VkImage _image;
			vkcheck(vkCreateImage(context->defaultDevice, &createInfo, context->m_allocation_callback, &_image));
			vkBindImageMemory(context->defaultDevice, _image, result->m_memory, result->m_MemoryOffset);
			VkImageView _view;
			viewInfo.image = _image;
			vkcheck(vkCreateImageView(context->defaultDevice, &viewInfo, context->m_allocation_callback, &_view));
			images.push_back(_image);
			views.push_back(_view);
		}
		result->m_images = images;
		result->m_views = views;
	}
	return result;
}

void Vulkan_GPUTexture2D_UploadPixels(IGPUTexture2D texture, void *pixels, size_t pixel_size)
{
	vk::VkContext context = (vk::VkContext)texture->m_Context;
	// create staging buffer
	IBuffer2 staging_buffer = Buffer2_Create(texture->m_Context, BufferType::INTE_TRANSFER_SRC, pixel_size, BufferMemoryType::DYNAMIC);
	// load data to buffer
	void *staging_buffer_ptr = Buffer2_Map(staging_buffer, false, true);
	memcpy(staging_buffer_ptr, pixels, pixel_size);
	Buffer2_Unmap(staging_buffer, true);
	// Create cmd_buffer and pool
	VkCommandPool pool = vk::Gfx_CreateCommandPool(context, true, false);
	VkCommandBuffer cmd_buffer = vk::Gfx_AllocCommandBuffer(context, pool, true);
	VkFence fence = vk::Gfx_CreateFence(context);
	// transition image into suitable layout
	VkAccessFlags CurrentAccessFlag = 0;
	VkPipelineStageFlags CurrentPipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.srcAccessMask = CurrentAccessFlag;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = texture->m_CurrentLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	texture->m_CurrentLayout = barrier.newLayout;
	CurrentAccessFlag = barrier.dstAccessMask;

	barrier.image = (VkImage)texture->m_NativeHandle;
	barrier.subresourceRange.aspectMask = texture->m_ImageAspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = (texture->m_Vk_AllocatedMipMaps) ? (std::floor(std::log2(std::max(texture->m_specification.m_Width, texture->m_specification.m_Height))) + 1) : 1;;
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
	vkCmdCopyBufferToImage(cmd_buffer, staging_buffer->m_vk_buffer->m_buffer, (VkImage)texture->m_NativeHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copy_region);

	// transfer image back to readable format
	VkImageMemoryBarrier barrier_readable = barrier;
	barrier_readable.srcAccessMask = CurrentAccessFlag;
	barrier_readable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier_readable.oldLayout = texture->m_CurrentLayout;
	barrier_readable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier_readable.subresourceRange.levelCount = (texture->m_Vk_AllocatedMipMaps) ? (std::floor(std::log2(std::max(texture->m_specification.m_Width, texture->m_specification.m_Height))) + 1) : 1;
	texture->m_CurrentLayout = barrier_readable.newLayout;
	CurrentAccessFlag = barrier_readable.dstAccessMask;
	vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &barrier_readable);
	CurrentAccessFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	// tell vulkan to do the work
	vkEndCommandBuffer(cmd_buffer);
	vk::Gfx_SubmitCmdBuffers(context->defaultQueue, {cmd_buffer}, {}, {}, {}, fence);
	vkWaitForFences(context->defaultDevice, 1, &fence, true, UINT64_MAX);
	// cleanup
	vkDestroyCommandPool(context->defaultDevice, pool, context->m_allocation_callback);
	vkDestroyFence(context->defaultDevice, fence, context->m_allocation_callback);
	Buffer2_Destroy(staging_buffer);

	if (texture->m_specification.m_GenerateMipMapLevels)
		GPUTexture2D_GenerateMips(texture);
}

// For pipeline barriers, useful
// https://vulkan.lunarg.com/doc/view/1.2.198.1/windows/1.2-extensions/vkspec.html#synchronization-access-types-supported
void Vulkan_GPUTexture2D_GenerateMips(IGPUTexture2D texture)
{
	assert(texture->m_Vk_AllocatedMipMaps && "Must allocate mipmaps in constructor! (set generate mips flag to true)");
	using namespace vk;

	uint32_t miplevels = std::floor(std::log2(std::max(texture->m_specification.m_Width, texture->m_specification.m_Height))) + 1;
	VkContext context = (VkContext)texture->m_Context;
	VkCommandPool pool = Gfx_CreateCommandPool(context, true, false);
	VkCommandBuffer cmd = Gfx_AllocCommandBuffer(context, pool, true);
	VkFence fence = Gfx_CreateFence(context);

	Gfx_StartCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageMemoryBarrier MIP0Barrier0{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	MIP0Barrier0.srcAccessMask = 0;
	MIP0Barrier0.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	MIP0Barrier0.oldLayout = texture->m_CurrentLayout;
	MIP0Barrier0.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	MIP0Barrier0.srcQueueFamilyIndex = MIP0Barrier0.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	MIP0Barrier0.image = (VkImage)texture->m_NativeHandle;
	MIP0Barrier0.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	MIP0Barrier0.subresourceRange.layerCount = 1;
	MIP0Barrier0.subresourceRange.baseMipLevel = 0;
	MIP0Barrier0.subresourceRange.levelCount = 1;

	VkImageMemoryBarrier MIPImageBarrier0{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	MIPImageBarrier0.srcAccessMask = 0;
	MIPImageBarrier0.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	MIPImageBarrier0.oldLayout = texture->m_CurrentLayoutMips;
	MIPImageBarrier0.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	MIPImageBarrier0.srcQueueFamilyIndex = MIPImageBarrier0.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	MIPImageBarrier0.image = (VkImage)texture->m_NativeHandle;
	MIPImageBarrier0.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	MIPImageBarrier0.subresourceRange.layerCount = 1;
	MIPImageBarrier0.subresourceRange.baseMipLevel = 1;
	MIPImageBarrier0.subresourceRange.levelCount = miplevels - 1;
	texture->m_CurrentLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	texture->m_CurrentLayoutMips = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	
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
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {SourceWidth, SourceHeight, 1};
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = {std::max(SourceWidth >> i, 1), std::max(SourceHeight >> i, 1), 1};
		vkCmdBlitImage(cmd, (VkImage)texture->m_NativeHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, (VkImage)texture->m_NativeHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
	}

	VkImageMemoryBarrier MIP0Barrier1{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	MIP0Barrier1.srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	MIP0Barrier1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	MIP0Barrier1.dstAccessMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	MIP0Barrier1.oldLayout = texture->m_CurrentLayout;
	MIP0Barrier1.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	MIP0Barrier1.srcQueueFamilyIndex = MIP0Barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	MIP0Barrier1.image = (VkImage)texture->m_NativeHandle;
	MIP0Barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	MIP0Barrier1.subresourceRange.layerCount = 1;
	MIP0Barrier1.subresourceRange.baseMipLevel = 0;
	MIP0Barrier1.subresourceRange.levelCount = 1;

	VkImageMemoryBarrier MIPImageBarrier1{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	MIPImageBarrier1.srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	MIPImageBarrier1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	MIPImageBarrier1.oldLayout = texture->m_CurrentLayoutMips;
	MIPImageBarrier1.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	MIPImageBarrier1.srcQueueFamilyIndex = MIPImageBarrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	MIPImageBarrier1.image = (VkImage)texture->m_NativeHandle;
	MIPImageBarrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	MIPImageBarrier1.subresourceRange.layerCount = 1;
	MIPImageBarrier1.subresourceRange.baseMipLevel = 1;
	MIPImageBarrier1.subresourceRange.levelCount = miplevels - 1;
	texture->m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texture->m_CurrentLayoutMips = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkImageMemoryBarrier Barriers2[2] = { MIP0Barrier1 , MIPImageBarrier1 };

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, Barriers2);

	vkEndCommandBuffer(cmd);
	Gfx_SubmitCmdBuffers(context->defaultQueue, {cmd}, {}, {}, {}, fence);
	vkWaitForFences(context->defaultDevice, 1, &fence, true, 5000'0000'000);

	vkDestroyFence(context->defaultDevice, fence, context->m_allocation_callback);
	vkDestroyCommandPool(context->defaultDevice, pool, context->m_allocation_callback);
}

APIHandle Vulkan_GPUTexture2D_GetImage(IGPUTexture2D texture)
{
	if (texture->m_specification.m_CreatePerFrame)
	{
		auto vcont = ToVKContext(texture->m_Context);
		int FrameIndex = vcont->FrameInfo->m_FrameIndex;
		return texture->m_images[FrameIndex];
	}
	else
	{
		return texture->m_NativeHandle;
	}
}

APIHandle Vulkan_GPUTexture2D_GetView(IGPUTexture2D texture)
{
	if (texture->m_specification.m_CreatePerFrame)
	{
		auto vcont = ToVKContext(texture->m_Context);
		int FrameIndex = vcont->FrameInfo->m_FrameIndex;
		return texture->m_views[FrameIndex];
	}
	else
	{
		return texture->view;
	}
}

void Vulkan_GPUTexture2D_Destroy(IGPUTexture2D texture)
{
	vk::VkContext context = (vk::VkContext)texture->m_Context;
	if (texture->m_CreatePerFrame)
	{
		vkDestroyImageView(context->defaultDevice, texture->m_views[0], context->m_allocation_callback);
		vulkan_memory_free_texture2d((VulkanMemoryContext)context->m_memory_context, texture->m_images[0], (uint32_t)texture->m_AllocationIndex);
		for (size_t i = 1; i < texture->m_views.size(); i++)
			vkDestroyImageView(context->defaultDevice, texture->m_views[i], context->m_allocation_callback);
		for (size_t i = 1; i < texture->m_images.size(); i++)
			vkDestroyImage(context->defaultDevice, texture->m_images[i], context->m_allocation_callback);
	}
	else
	{
		vkDestroyImageView(context->defaultDevice, texture->view, context->m_allocation_callback);
		vulkan_memory_free_texture2d((VulkanMemoryContext)context->m_memory_context, (VkImage)texture->m_NativeHandle, (uint32_t)texture->m_AllocationIndex);
	}
	delete texture;
}

// =================================== OPENGL ===================================

IGPUTexture2D GL_GPUTexture2D_Create(GraphicsContext context, Texture2DSpecification *_specification)
{
	Texture2DSpecification specification = *_specification;
	GPUTexture2D *result = new GPUTexture2D();
	result->m_Context = context;
	result->m_specification = specification;
	result->m_Vk_AllocatedMipMaps = specification.m_GenerateMipMapLevels;
	result->m_MipLevels = specification.m_GenerateMipMapLevels ? std::floor(std::log2(std::max(specification.m_Width, specification.m_Height))) + 1 : 1;
	result->m_CreatePerFrame = specification.m_CreatePerFrame;
	if (specification.m_Samples != TextureSamples::MSAA_1)
	{
		if (specification.m_GenerateMipMapLevels)
		{
			logwarning("Cannot use mipmaps on multisampled textures!");
			specification.m_GenerateMipMapLevels = false;
			result->m_Vk_AllocatedMipMaps = false;
		}
	}
	if (specification.m_TextureUsage == TextureUsage::COLOR_ATTACHMENT && specification.m_GenerateMipMapLevels)
		logwarning("Creating Tex2D Color Attachment with MIPMAPs");

	assert(context && specification.m_Width > 0 && specification.m_Height > 0);
	char api = *(char *)context;
	result->m_ApiType = 1;

	int SampleCount;
	switch (specification.m_Samples)
	{
	case TextureSamples::MSAA_1:
		SampleCount = 1;
		break;
	case TextureSamples::MSAA_2:
		SampleCount = 2;
		break;
	case TextureSamples::MSAA_4:
		SampleCount = 4;
		break;
	case TextureSamples::MSAA_8:
		SampleCount = 8;
		break;
	case TextureSamples::MSAA_16:
		SampleCount = 16;
		break;
	case TextureSamples::MSAA_32:
		SampleCount = 32;
		break;
	case TextureSamples::MSAA_64:
		SampleCount = 64;
		break;
	default:
		SampleCount = 1;
		logalert("Invalid TextureSample Count in EngineTexture2D::Create(...)");
	}

	auto GetTarget = [](uint32_t Multisample) throw()->GLenum { return (Multisample <= 1) ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE; };
	auto GetWrap = [](TextureWrapping w) throw()->GLenum
	{
		if (w == TextureWrapping::ClampToBorder)
			return GL_CLAMP_TO_BORDER;
		else if (w == TextureWrapping::ClampToEdge)
			return GL_CLAMP_TO_EDGE;
		else if (w == TextureWrapping::MirroredRepeat)
			return GL_MIRRORED_REPEAT;
		return GL_REPEAT;
	};
	auto GetSwizzle = [](TextureSwizzle swizz, GLenum component) throw()->GLenum
	{
		if (swizz == TextureSwizzle::Identity)
			return component;
		else if (swizz == TextureSwizzle::SwizzleALPHA)
			return GL_ALPHA;
		else if (swizz == TextureSwizzle::SwizzleBLUE)
			return GL_BLUE;
		else if (swizz == TextureSwizzle::SwizzleGREEN)
			return GL_GREEN;
		else if (swizz == TextureSwizzle::SwizzleRED)
			return GL_RED;
		else if (swizz == TextureSwizzle::SwizzleONE)
			return GL_ONE;
		else if (swizz == TextureSwizzle::SwizzleZERO)
			return GL_ZERO;
		return component;
	};
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GetTarget(SampleCount), textureID);
	switch (specification.m_Sampler.MagFilter)
	{
	case TextureFilter::Nearest:
		glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case TextureFilter::Linear:
		glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}
	switch (specification.m_Sampler.MinFilter)
	{
	case TextureFilter::Nearest:
		glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case TextureFilter::Linear:
		glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	}
	// first is texture sampling and second is mipmap sampling
	if (specification.m_GenerateMipMapLevels)
		switch (specification.m_Sampler.MIPMAPFilter)
		{
		case TextureMIPFilter::MIPMAP_Linear:
			if (specification.m_Sampler.MinFilter == TextureFilter::Linear)
			{
				glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
			{
				glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
			}
			break;
		case TextureMIPFilter::MIPMAP_Nearest:
			if (specification.m_Sampler.MinFilter == TextureFilter::Linear)
			{
				glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			}
			break;
		}
	glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_WRAP_S, GetWrap(specification.m_Sampler.WrappingMode_S));
	glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_WRAP_T, GetWrap(specification.m_Sampler.WrappingMode_T));
	glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_WRAP_R, GetWrap(specification.m_Sampler.WrappingMode_R));
	glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_SWIZZLE_R, GetSwizzle(specification.m_Sampler.SwizzleRed, GL_RED));
	glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_SWIZZLE_G, GetSwizzle(specification.m_Sampler.SwizzleGreen, GL_GREEN));
	glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_SWIZZLE_B, GetSwizzle(specification.m_Sampler.SwizzleBlue, GL_BLUE));
	glTexParameteri(GetTarget(SampleCount), GL_TEXTURE_SWIZZLE_A, GetSwizzle(specification.m_Sampler.SwizzleAlpha, GL_ALPHA));
	glTexParameterf(GetTarget(SampleCount), GL_TEXTURE_LOD_BIAS, specification.m_Sampler.MipMapLODBias);
	glTexParameterf(GetTarget(SampleCount), GL_TEXTURE_MIN_LOD, specification.m_Sampler.MinLOD);
	glTexParameterf(GetTarget(SampleCount), GL_TEXTURE_MAX_LOD, specification.m_Sampler.MaxLOD);
	if (specification.m_Sampler.EnableAnisotropicFiltering)
	{
		GLint MaxAnistropy;
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &MaxAnistropy);
		float AnsitropyLevel = std::min(std::max(1.0f, specification.m_Sampler.MaxAnisotropicLevel), (float)MaxAnistropy);
		glTexParameterf(GetTarget(SampleCount), GL_TEXTURE_MAX_ANISOTROPY_EXT, AnsitropyLevel);
	}
	// Get Format
	GLenum internalFormat = GL_RGBA;
	GLenum glformat = GL_RGBA;
	GLenum type = GL_UNSIGNED_BYTE;
	GetOpenGLTextureFormatType(specification.m_Format, internalFormat, glformat, type);
	glTexImage2D(GetTarget(SampleCount), 0, internalFormat, specification.m_Width, specification.m_Height, 0, glformat, type, NULL);
	glBindTexture(GetTarget(SampleCount), 0);
	result->m_NativeHandle = UInt32ToPVOID(textureID);

	return result;
}

void GL_GPUTexture2D_UploadPixels(IGPUTexture2D texture, void *pixels, size_t pixel_size)
{
	glBindTexture(GL_TEXTURE_2D, PVOIDToUInt32(texture->m_NativeHandle));
	// Get Format
	GLenum internalFormat = GL_RGBA;
	GLenum glformat = GL_RGBA;
	GLenum type = GL_UNSIGNED_BYTE;
	GetOpenGLTextureFormatType(texture->m_specification.m_Format, internalFormat, glformat, type);
	if (texture->m_specification.m_TextureUsage == TextureUsage::DEPTH_ATTACHMENT)
		type = GL_DEPTH_COMPONENT;
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texture->m_specification.m_Width, texture->m_specification.m_Height, 0, glformat, type, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);
	if (texture->m_specification.m_GenerateMipMapLevels)
		GPUTexture2D_GenerateMips(texture);
}

void GL_GPUTexture2D_GenerateMips(IGPUTexture2D texture)
{
	assert(texture->m_Vk_AllocatedMipMaps && "Must allocate mipmaps in constructor! (set generate mips flag to true)");
	glBindTexture(GL_TEXTURE_2D, (uint64_t)texture->m_NativeHandle);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

APIHandle GL_GPUTexture2D_GetImage(IGPUTexture2D texture)
{
	return texture->m_NativeHandle;
}

APIHandle GL_GPUTexture2D_GetView(IGPUTexture2D texture)
{
	return nullptr;
}

void GL_GPUTexture2D_Destroy(IGPUTexture2D texture)
{
	glDeleteTextures(1, (GLuint *)&texture->m_NativeHandle);
}

void GPUTexture2D_LinkFunctions(GraphicsContext context)
{
	char ApiType = *(char *)context;
	if (ApiType == 0)
	{
		GPUTexture2D_Create = Vulkan_GPUTexture2D_Create;
		GPUTexture2D_UploadPixels = Vulkan_GPUTexture2D_UploadPixels;
		GPUTexture2D_GenerateMips = Vulkan_GPUTexture2D_GenerateMips;
		GPUTexture2D_GetImage = Vulkan_GPUTexture2D_GetImage;
		GPUTexture2D_GetView = Vulkan_GPUTexture2D_GetView;
		GPUTexture2D_Destroy = Vulkan_GPUTexture2D_Destroy;
	}
	else if (ApiType == 1)
	{
		GPUTexture2D_Create = GL_GPUTexture2D_Create;
		GPUTexture2D_UploadPixels = GL_GPUTexture2D_UploadPixels;
		GPUTexture2D_GenerateMips = GL_GPUTexture2D_GenerateMips;
		GPUTexture2D_GetImage = GL_GPUTexture2D_GetImage;
		GPUTexture2D_GetView = GL_GPUTexture2D_GetView;
		GPUTexture2D_Destroy = GL_GPUTexture2D_Destroy;
	}
	else
	{
		assert(0);
		Utils::Break();
	}
}

IGPUTexture2D GPUTexture2D_CreateFromFile(GraphicsContext context, const char* path, bool GenerateMipMaps)
{
	int w, h, c;
	uint32_t* pixels = (uint32_t*)stbi_load(path, &w, &h, &c, 4);
	if(!pixels)
	{
		std::stringstream ss;
		if(!Utils::Exists(path))
			ss << "Failed to create GPUTexture2D from file " << path << " because image could not be found.";
		else
			ss << "Failed to create GPUTexture2D from file " << path << " because image is corrupted or format is not supported.";
		logerrors(ss.str());
		return nullptr;
	}
	Texture2DSpecification specification;
	specification.m_Width = w;
	specification.m_Height = h;
	specification.m_TextureUsage = TextureUsage::DEFAULT;
	specification.m_Samples = TextureSamples::MSAA_1;
	specification.m_Format = TextureFormat::RGBA8;
	specification.m_GenerateMipMapLevels = GenerateMipMaps;
	specification.m_Sampler.EnableAnisotropicFiltering = true;
	specification.m_Sampler.MaxAnisotropicLevel = 16.0f;
	IGPUTexture2D texture = GPUTexture2D_Create(context, &specification);
	GPUTexture2D_UploadPixels(texture, pixels, w * h * sizeof(uint32_t));
	stbi_image_free(pixels);
	return texture;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Sampler ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
PFN_GPUTextureSampler_Create *GPUTextureSampler_Create = nullptr;
PFN_GPUTextureSampler_Destroy *GPUTextureSampler_Destroy = nullptr;

IGPUTextureSampler Vulkan_GPUTextureSampler_Create(GraphicsContext context, TextureSamplerSpecification *specification)
{
	TextureSamplerSpecification sampler = *specification;
	GPUTextureSampler *result = new GPUTextureSampler();
	result->m_Context = context;
	result->m_Sampler = sampler;
	vk::VkContext cont = (vk::VkContext)context;
	auto GetAddressMode = [](TextureWrapping s) throw()->VkSamplerAddressMode
	{
		if (s == TextureWrapping::ClampToBorder)
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		else if (s == TextureWrapping::ClampToEdge)
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		else if (s == TextureWrapping::MirroredRepeat)
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	};
	VkSamplerAddressMode addressModeU = GetAddressMode(result->m_Sampler.WrappingMode_U);
	VkSamplerAddressMode addressModeV = GetAddressMode(result->m_Sampler.WrappingMode_V);
	VkSamplerAddressMode addressModeW = GetAddressMode(result->m_Sampler.WrappingMode_W);
	float AnisotropyLevel = std::min(std::max(1.0f, sampler.MaxAnisotropicLevel), cont->card.deviceLimits.maxSamplerAnisotropy);
	VkSamplerCreateInfo createInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	createInfo.magFilter = (sampler.MagFilter == TextureFilter::Linear) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	createInfo.minFilter = (sampler.MinFilter == TextureFilter::Linear) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	createInfo.mipmapMode = (sampler.MIPMAPFilter == TextureMIPFilter::MIPMAP_Linear) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	createInfo.addressModeU = addressModeU;
	createInfo.addressModeV = addressModeV;
	createInfo.addressModeW = addressModeW;
	createInfo.mipLodBias = sampler.MipMapLODBias;
	createInfo.anisotropyEnable = (sampler.EnableAnisotropicFiltering) ? VK_TRUE : VK_FALSE;
	createInfo.maxAnisotropy = AnisotropyLevel;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.minLod = sampler.MinLOD;
	createInfo.maxLod = sampler.MaxLOD;
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	VkSampler _sampler;
	vkcheck(vkCreateSampler(cont->defaultDevice, &createInfo, cont->m_allocation_callback, &_sampler));
	result->m_NativeHandle = _sampler;
	return result;
}

void Vulkan_GPUTextureSampler_Destroy(IGPUTextureSampler sampler)
{
	vk::VkContext context = (vk::VkContext)sampler->m_Context;
	vkDestroySampler(context->defaultDevice, (VkSampler)sampler->m_NativeHandle, context->m_allocation_callback);
	delete sampler;
}

IGPUTextureSampler GL_GPUTextureSampler_Create(GraphicsContext context, TextureSamplerSpecification *specification)
{
	TextureSamplerSpecification sampler = *specification;
	GPUTextureSampler *result = new GPUTextureSampler();
	result->m_Context = context;
	result->m_Sampler = sampler;
	auto GetWrap = [](TextureWrapping w) throw()->GLenum
	{
		if (w == TextureWrapping::ClampToBorder)
			return GL_CLAMP_TO_BORDER;
		else if (w == TextureWrapping::ClampToEdge)
			return GL_CLAMP_TO_EDGE;
		else if (w == TextureWrapping::MirroredRepeat)
			return GL_MIRRORED_REPEAT;
		return GL_REPEAT;
	};
	GLuint samplerID;
	glGenSamplers(1, &samplerID);
	switch (result->m_Sampler.MagFilter)
	{
	case TextureFilter::Nearest:
		glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case TextureFilter::Linear:
		glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}
	// first is texture sampling and second is mipmap sampling
	switch (result->m_Sampler.MIPMAPFilter)
	{
	case TextureMIPFilter::MIPMAP_Linear:
		if (result->m_Sampler.MinFilter == TextureFilter::Linear)
		{
			glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
		{
			glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		}
		break;
	case TextureMIPFilter::MIPMAP_Nearest:
		if (result->m_Sampler.MinFilter == TextureFilter::Linear)
		{
			glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		}
		else
		{
			glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
		break;
	}
	glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_S, GetWrap(result->m_Sampler.WrappingMode_S));
	glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_T, GetWrap(result->m_Sampler.WrappingMode_T));
	glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_R, GetWrap(result->m_Sampler.WrappingMode_R));
	glSamplerParameteri(samplerID, GL_TEXTURE_LOD_BIAS, result->m_Sampler.MipMapLODBias);
	glSamplerParameteri(samplerID, GL_TEXTURE_MIN_LOD, result->m_Sampler.MinLOD);
	glSamplerParameteri(samplerID, GL_TEXTURE_MAX_LOD, result->m_Sampler.MaxLOD);
	if (result->m_Sampler.EnableAnisotropicFiltering)
	{
		GLint MaxAnistropy;
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &MaxAnistropy);
		float AnsitropyLevel = std::min(std::max(1.0f, sampler.MaxAnisotropicLevel), (float)MaxAnistropy);
		glSamplerParameterf(samplerID, GL_TEXTURE_MAX_ANISOTROPY, AnsitropyLevel);
	}
	result->m_NativeHandle = (void *)uint64_t(samplerID);
	return result;
}

void GL_GPUTextureSampler_Destroy(IGPUTextureSampler sampler)
{
	glDeleteSamplers(1, (GLuint *)&sampler->m_NativeHandle);
	delete sampler;
}

IGPUTextureSampler GPUTextureSampler_CreateDefaultSampler(GraphicsContext context)
{
	TextureSamplerSpecification specification;
	specification.EnableAnisotropicFiltering = true;
	specification.MaxAnisotropicLevel = 16.0f;
	IGPUTextureSampler sampler = GPUTextureSampler_Create(context, &specification);
	return sampler;
}

void GPUTextureSampler_LinkFunctions(GraphicsContext context)
{
	char ApiType = *(char *)context;
	if (ApiType == 0)
	{
		GPUTextureSampler_Create = Vulkan_GPUTextureSampler_Create;
		GPUTextureSampler_Destroy = Vulkan_GPUTextureSampler_Destroy;
	}
	else if (ApiType == 1)
	{
		GPUTextureSampler_Create = GL_GPUTextureSampler_Create;
		GPUTextureSampler_Destroy = GL_GPUTextureSampler_Destroy;
	}
	else
	{
		assert(0);
		Utils::Break();
	}
}

static void GetOpenGLTextureFormatType(TextureFormat format, GLenum &OutInternalFormat, GLenum &OutFormat, GLenum &OutDataType)
{
	if (format == TextureFormat::R8)
	{
		OutInternalFormat = GL_R8;
		OutFormat = GL_RED;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::R8_SNORM)
	{
		OutInternalFormat = GL_R8_SNORM;
		OutFormat = GL_RED;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::R16)
	{
		OutInternalFormat = GL_R16;
		OutFormat = GL_RED;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::R16_SNORM)
	{
		OutInternalFormat = GL_R16_SNORM;
		OutFormat = GL_RED;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RG8)
	{
		OutInternalFormat = GL_RG8;
		OutFormat = GL_RG;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RG8_SNORM)
	{
		OutInternalFormat = GL_RG8_SNORM;
		OutFormat = GL_RG;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RG16)
	{
		OutInternalFormat = GL_RG16;
		OutFormat = GL_RG;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RG16_SNORM)
	{
		OutInternalFormat = GL_RG16_SNORM;
		OutFormat = GL_RG;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGB8)
	{
		OutInternalFormat = GL_RGB8;
		OutFormat = GL_RGB;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RGB8_SNORM)
	{
		OutInternalFormat = GL_RGB8_SNORM;
		OutFormat = GL_RGB;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGB16_SNORM)
	{
		OutInternalFormat = GL_RGB16_SNORM;
		OutFormat = GL_RGB;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA8)
	{
		OutInternalFormat = GL_RGBA8;
		OutFormat = GL_RGBA;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA8_SNORM)
	{
		OutInternalFormat = GL_RGBA8_SNORM;
		OutFormat = GL_RGBA;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA16)
	{
		OutInternalFormat = GL_RGBA16;
		OutFormat = GL_RGBA;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::SRGB8)
	{
		OutInternalFormat = GL_SRGB8;
		OutFormat = GL_RGB;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::SRGB8_ALPHA8)
	{
		OutInternalFormat = GL_SRGB8_ALPHA8;
		OutFormat = GL_RGBA;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::R16F)
	{
		OutInternalFormat = GL_R16F;
		OutFormat = GL_RED;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::RG16F)
	{
		OutInternalFormat = GL_RG16F;
		OutFormat = GL_RG;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::RGB16F)
	{
		OutInternalFormat = GL_RGB16F;
		OutFormat = GL_RGB;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::RGBA16F)
	{
		OutInternalFormat = GL_RGBA16F;
		OutFormat = GL_RGBA;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::R32F)
	{
		OutInternalFormat = GL_R32F;
		OutFormat = GL_RED;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::D32F)
	{
		OutInternalFormat = GL_DEPTH_COMPONENT32;
		OutFormat = GL_DEPTH_COMPONENT;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::RG32F)
	{
		OutInternalFormat = GL_RG32F;
		OutFormat = GL_RG;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::RGB32F)
	{
		OutInternalFormat = GL_RGB32F;
		OutFormat = GL_RGB;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::RGBA32F)
	{
		OutInternalFormat = GL_RGBA32F;
		OutFormat = GL_RGBA;
		OutDataType = GL_FLOAT;
		return;
	}
	if (format == TextureFormat::R8I)
	{
		OutInternalFormat = GL_R8I;
		OutFormat = GL_RED;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::R8UI)
	{
		OutInternalFormat = GL_R8UI;
		OutFormat = GL_RED;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::R16I)
	{
		OutInternalFormat = GL_R16I;
		OutFormat = GL_RED;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::R16UI)
	{
		OutInternalFormat = GL_R16UI;
		OutFormat = GL_RED;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::R32I)
	{
		OutInternalFormat = GL_R32I;
		OutFormat = GL_RED;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::R32UI)
	{
		OutInternalFormat = GL_R32UI;
		OutFormat = GL_RED;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RG8I)
	{
		OutInternalFormat = GL_RG8I;
		OutFormat = GL_RG;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RG8UI)
	{
		OutInternalFormat = GL_RG8UI;
		OutFormat = GL_RG;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RG16I)
	{
		OutInternalFormat = GL_RG16I;
		OutFormat = GL_RG;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RG16UI)
	{
		OutInternalFormat = GL_RG16UI;
		OutFormat = GL_RG;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RG32I)
	{
		OutInternalFormat = GL_RG32I;
		OutFormat = GL_RG;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RG32UI)
	{
		OutInternalFormat = GL_RG32UI;
		OutFormat = GL_RG;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RGB8I)
	{
		OutInternalFormat = GL_RGB8I;
		OutFormat = GL_RGB;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGB8UI)
	{
		OutInternalFormat = GL_RGB8UI;
		OutFormat = GL_RGB;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RGB16I)
	{
		OutInternalFormat = GL_RGB16I;
		OutFormat = GL_RGB;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGB16UI)
	{
		OutInternalFormat = GL_RGB16UI;
		OutFormat = GL_RGB;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RGB32I)
	{
		OutInternalFormat = GL_RGB32I;
		OutFormat = GL_RGB;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGB32UI)
	{
		OutInternalFormat = GL_RGB32UI;
		OutFormat = GL_RGB;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA8I)
	{
		OutInternalFormat = GL_RGBA8I;
		OutFormat = GL_RGBA;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA8UI)
	{
		OutInternalFormat = GL_RGBA8UI;
		OutFormat = GL_RGBA;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA16I)
	{
		OutInternalFormat = GL_RGBA16I;
		OutFormat = GL_RGBA;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA16UI)
	{
		OutInternalFormat = GL_RGBA16UI;
		OutFormat = GL_RGBA;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA32I)
	{
		OutInternalFormat = GL_RGBA32I;
		OutFormat = GL_RGBA;
		OutDataType = GL_BYTE;
		return;
	}
	if (format == TextureFormat::RGBA32UI)
	{
		OutInternalFormat = GL_RGBA32UI;
		OutFormat = GL_RGBA;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::BGRA8)
	{
		OutInternalFormat = GL_BGRA;
		OutFormat = GL_BGRA;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::SBGR8_ALPHA8)
	{
		OutInternalFormat = GL_BGRA;
		OutFormat = GL_BGRA;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}
	if (format == TextureFormat::R11G11B10F)
	{
		OutInternalFormat = GL_R11F_G11F_B10F;
		OutFormat = GL_RGB;
		OutDataType = GL_UNSIGNED_BYTE;
		return;
	}

	assert(0 && "Unsupported Conversion from TextureFormat to OpenGL Texture Format.");
}

TextureFormat Textures_StringToTextureFormat(std::string &_input)
{
	std::string input = Utils::StrUpperCase(_input);
	if (Utils::StrContains(input, "UNDEFINED"))
		return TextureFormat::UNDEFINED;
	if (Utils::StrContains(input, "R8"))
		return TextureFormat::R8;
	if (Utils::StrContains(input, "R8_SNORM"))
		return TextureFormat::R8_SNORM;
	if (Utils::StrContains(input, "R16"))
		return TextureFormat::R16;
	if (Utils::StrContains(input, "R16_SNORM"))
		return TextureFormat::R16_SNORM;
	if (Utils::StrContains(input, "RG8"))
		return TextureFormat::RG8;
	if (Utils::StrContains(input, "RG8_SNORM"))
		return TextureFormat::RG8_SNORM;
	if (Utils::StrContains(input, "RG16"))
		return TextureFormat::RG16;
	if (Utils::StrContains(input, "RG16_SNORM"))
		return TextureFormat::RG16_SNORM;
	if (Utils::StrContains(input, "RGB8"))
		return TextureFormat::RGB8;
	if (Utils::StrContains(input, "RGB8_SNORM"))
		return TextureFormat::RGB8_SNORM;
	if (Utils::StrContains(input, "RGB16_SNORM"))
		return TextureFormat::RGB16_SNORM;
	if (Utils::StrContains(input, "RGBA8"))
		return TextureFormat::RGBA8;
	if (Utils::StrContains(input, "RGBA8_SNORM"))
		return TextureFormat::RGBA8_SNORM;
	if (Utils::StrContains(input, "RGBA16"))
		return TextureFormat::RGBA16;
	if (Utils::StrContains(input, "SRGB8"))
		return TextureFormat::SRGB8;
	if (Utils::StrContains(input, "SRGB8_ALPHA8"))
		return TextureFormat::SRGB8_ALPHA8;
	if (Utils::StrContains(input, "R16F"))
		return TextureFormat::R16F;
	if (Utils::StrContains(input, "RG16F"))
		return TextureFormat::RG16F;
	if (Utils::StrContains(input, "RGB16F"))
		return TextureFormat::RGB16F;
	if (Utils::StrContains(input, "RGBA16F"))
		return TextureFormat::RGBA16F;
	if (Utils::StrContains(input, "R32F"))
		return TextureFormat::R32F;
	if (Utils::StrContains(input, "D32"))
		return TextureFormat::D32F;
	if (Utils::StrContains(input, "RG32F"))
		return TextureFormat::RG32F;
	if (Utils::StrContains(input, "RGB32F"))
		return TextureFormat::RGB32F;
	if (Utils::StrContains(input, "RGBA32F"))
		return TextureFormat::RGBA32F;
	if (Utils::StrContains(input, "R8I"))
		return TextureFormat::R8I;
	if (Utils::StrContains(input, "R8UI"))
		return TextureFormat::R8UI;
	if (Utils::StrContains(input, "R16I"))
		return TextureFormat::R16I;
	if (Utils::StrContains(input, "R16UI"))
		return TextureFormat::R16UI;
	if (Utils::StrContains(input, "R32I"))
		return TextureFormat::R32I;
	if (Utils::StrContains(input, "R32UI"))
		return TextureFormat::R32UI;
	if (Utils::StrContains(input, "RG8I"))
		return TextureFormat::RG8I;
	if (Utils::StrContains(input, "RG8UI"))
		return TextureFormat::RG8UI;
	if (Utils::StrContains(input, "RG16I"))
		return TextureFormat::RG16I;
	if (Utils::StrContains(input, "RG16UI"))
		return TextureFormat::RG16UI;
	if (Utils::StrContains(input, "RG32I"))
		return TextureFormat::RG32I;
	if (Utils::StrContains(input, "RG32UI"))
		return TextureFormat::RG32UI;
	if (Utils::StrContains(input, "RGB8I"))
		return TextureFormat::RGB8I;
	if (Utils::StrContains(input, "RGB8UI"))
		return TextureFormat::RGB8UI;
	if (Utils::StrContains(input, "RGB16I"))
		return TextureFormat::RGB16I;
	if (Utils::StrContains(input, "RGB16UI"))
		return TextureFormat::RGB16UI;
	if (Utils::StrContains(input, "RGB32I"))
		return TextureFormat::RGB32I;
	if (Utils::StrContains(input, "RGB32UI"))
		return TextureFormat::RGB32UI;
	if (Utils::StrContains(input, "RGBA8I"))
		return TextureFormat::RGBA8I;
	if (Utils::StrContains(input, "RGBA8UI"))
		return TextureFormat::RGBA8UI;
	if (Utils::StrContains(input, "RGBA16I"))
		return TextureFormat::RGBA16I;
	if (Utils::StrContains(input, "RGBA16UI"))
		return TextureFormat::RGBA16UI;
	if (Utils::StrContains(input, "RGBA32I"))
		return TextureFormat::RGBA32I;
	if (Utils::StrContains(input, "RGBA32UI"))
		return TextureFormat::RGBA32UI;
	if (Utils::StrContains(input, "BGRA8"))
		return TextureFormat::BGRA8;
	if (Utils::StrContains(input, "SBGR8_ALPHA8"))
		return TextureFormat::SBGR8_ALPHA8;
	if (Utils::StrContains(input, "R11G11B10F"))
		return TextureFormat::R11G11B10F;
	std::string error_message = "Could not parse string format '" + input + "'";
	logerror(error_message.c_str());
	assert(0);
	return TextureFormat::UNDEFINED;
}
