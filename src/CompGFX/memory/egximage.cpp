#include "egximage.hpp"
#include "formatsize.hpp"
#include <core/ScopedCommandBuffer.hpp>
#include <imgui/backends/imgui_impl_vulkan.h>

using namespace egx;
using namespace std;

Image2D::Image2D(const DeviceCtx& pCtx, int width, int height, vk::Format format, int mipLevels, vk::ImageUsageFlags usage, vk::ImageLayout initialLayout, bool streaming)
	: Width(width), Height(height), Format(format), StreamingMode(streaming), Usage(usage), CurrentLayout(vk::ImageLayout::eUndefined)
{
	int maxMipLevels = (int)std::min(std::log2(width), std::log2(height));
	if (mipLevels <= 0) {
		m_MipLevels = maxMipLevels;
	}
	else {
		m_MipLevels = std::min(mipLevels, maxMipLevels);
	}

	m_Data = std::make_shared<Image2D::DataWrapper>();
	m_Data->m_Ctx = pCtx;

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = VkFormat(format);
	createInfo.extent = { (uint32_t)width, (uint32_t)height, 1 };
	createInfo.mipLevels = m_MipLevels;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VkImageUsageFlags(usage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst);
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage handle;
	vk::Result result = (vk::Result)vmaCreateImage(pCtx->Allocator, &createInfo, &allocCreateInfo, &handle, &m_Data->m_Allocation, nullptr);
	if (result != vk::Result::eSuccess) {
		auto error = cpp::Format("Could not create image {}x{} format={} mipLevels={} usage={} initialLayout={}, VkResult={}",
			width, height, vk::to_string(format), vk::to_string(usage), vk::to_string(initialLayout), vk::to_string(result));
		throw std::runtime_error(error);
	}

	m_Data->m_Image = handle;
	m_TexelBytes = egx::FormatByteCount(VkFormat(format));

	size_t size = (static_cast<size_t>(width) * m_TexelBytes) * height;
	m_Data->m_StageBuffer = std::make_unique<Buffer>(m_Data->m_Ctx, size, egx::MemoryPreset::HostOnly, egx::HostMemoryAccess::Sequential, vk::BufferUsageFlagBits::eTransferSrc, false);
	SetLayout(initialLayout);
}

Image2D egx::Image2D::CreateFromHandle(const DeviceCtx& pCtx, vk::Image handle, int width, int height, vk::Format format, int mipLevels, vk::ImageUsageFlags usage, vk::ImageLayout initalLayout)
{
	Image2D image;
	image.Width = width;
	image.Height = height;
	image.Format = format;
	image.StreamingMode = false;
	image.Usage = usage;
	image.CurrentLayout = initalLayout;
	image.m_Data = std::make_shared<Image2D::DataWrapper>();
	image.m_Data->m_Image = handle;
	image.m_Data->m_Ctx = pCtx;
	return image;
}

void Image2D::SetPixel(int mipLevel, int x, int y, const void* pixelData)
{
	SetImageData(mipLevel, x, y, 1, 1, pixelData);
}

void Image2D::GetPixel(int mipLevel, int x, int y)
{
	Read(mipLevel, x, y, 1, 1, nullptr);
}

void Image2D::SetImageData(int mipLevel, int xOffset, int yOffset, int width, int height, const void* pData)
{
	ScopedCommandBuffer cmd(m_Data->m_Ctx);
	m_Data->m_StageBuffer->Write(pData, 0, (width * m_TexelBytes) * height);

	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_NONE;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VkImageLayout(CurrentLayout);
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_Data->m_Image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	vkCmdPipelineBarrier(cmd.Get(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	VkBufferImageCopy region{};
	region.imageOffset = { (int32_t)xOffset, (int32_t)yOffset, (int32_t)0 };
	region.imageExtent = { (uint32_t)width, (uint32_t)height, 1 };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;
	vkCmdCopyBufferToImage(cmd.Get(), m_Data->m_StageBuffer->GetHandle(), m_Data->m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VkImageLayout(CurrentLayout);
	barrier.srcAccessMask = barrier.dstAccessMask;
	barrier.dstAccessMask = VK_ACCESS_NONE;
	vkCmdPipelineBarrier(cmd.Get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Image2D::SetImageData(int mipLevel, const void* pData)
{
	SetImageData(mipLevel, 0, 0, Width, Height, pData);
}

void Image2D::Read(int mipLevel, int xOffset, int yOffset, int width, int height, void* pOutBuffer)
{
	ScopedCommandBuffer cmd(m_Data->m_Ctx);
	cmd->pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlagBits(0), {}, {}, vk::ImageMemoryBarrier(
			vk::AccessFlagBits::eNone,
			vk::AccessFlagBits::eTransferRead,
			CurrentLayout,
			vk::ImageLayout::eTransferSrcOptimal,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			m_Data->m_Image,
			vk::ImageSubresourceRange(
				GetFormatAspectFlags(Format),
				mipLevel,
				1,
				0,
				1
			)
		));

	vk::BufferImageCopy region;

	region.imageOffset.x = xOffset;
	region.imageOffset.y = yOffset;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	region.imageSubresource.aspectMask = GetFormatAspectFlags(Format);
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;

	cmd->copyImageToBuffer(m_Data->m_Image, vk::ImageLayout::eTransferSrcOptimal, m_Data->m_StageBuffer->GetHandle(), region);

	if (CurrentLayout == vk::ImageLayout::eUndefined) {
		CurrentLayout = vk::ImageLayout::eGeneral;
	}

	cmd->pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlagBits(0), {}, {}, vk::ImageMemoryBarrier(
			vk::AccessFlagBits::eTransferRead,
			vk::AccessFlagBits::eNone,
			vk::ImageLayout::eTransferSrcOptimal,
			CurrentLayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			m_Data->m_Image,
			vk::ImageSubresourceRange(
				GetFormatAspectFlags(Format),
				mipLevel,
				1,
				0,
				1
			)
		));

	cmd.RunNow();

	size_t memorySize = (static_cast<size_t>(width) * m_TexelBytes) * height;
	egx::MemoryMappedScope scope(*m_Data->m_StageBuffer.get());
	memcpy(pOutBuffer, scope.Ptr, memorySize);
}
void Image2D::Read(int mipLevel, void* pOutBuffer)
{
	Read(mipLevel, 0, 0, Width, Height, pOutBuffer);
}

void Image2D::GenerateMipmaps()
{
	// To generate mipmaps we must 
	// 1) Image Memory Barrier first mip to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMIAL
	// 2) Image Memory Barrier for all other mips to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMIAL
	// 3) Execute Linear Image Blit from first mip to all other mips
	// 4) Image Memory Barrier for first mip from VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL to CurrentLayout
	// 5) Image Memory Barrier for all other mips from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to CurrentLayout

	if (m_MipLevels <= 1) return;

	ScopedCommandBuffer cmd(m_Data->m_Ctx);
	// Step: 1
	auto srcMipBarrier = vk::ImageMemoryBarrier(
		vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eMemoryRead,
		CurrentLayout,
		vk::ImageLayout::eTransferSrcOptimal,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		m_Data->m_Image,
		vk::ImageSubresourceRange(
			GetFormatAspectFlags(Format),
			0,
			1,
			0,
			1
		)
	);
	cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlagBits(0), {}, {}, srcMipBarrier);

	// Step: 2
	auto dstMipBarrier = vk::ImageMemoryBarrier(
		vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eMemoryWrite,
		CurrentLayout,
		vk::ImageLayout::eTransferDstOptimal,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		m_Data->m_Image,
		vk::ImageSubresourceRange(
			GetFormatAspectFlags(Format),
			1,
			VK_REMAINING_MIP_LEVELS,
			0,
			1
		)
	);
	cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlagBits(0), {}, {}, dstMipBarrier);

	// Step: 3
	std::vector<vk::ImageBlit> regions(m_MipLevels - 1);
	for (int i = 1; i < m_MipLevels; i++) {
		// Absurd visual studio intellisense and compiler, how will a int minus int overflow?
		auto& region = regions[static_cast<std::vector<vk::ImageBlit, std::allocator<vk::ImageBlit>>::size_type>(i) - 1];
		region.srcSubresource.aspectMask = GetFormatAspectFlags(Format);
		region.srcSubresource.layerCount = 1;
		region.srcOffsets[1].x = Width;
		region.srcOffsets[1].y = Height;
		region.srcOffsets[1].z = 1;

		region.dstSubresource.aspectMask = GetFormatAspectFlags(Format);
		region.dstSubresource.mipLevel = i;
		region.dstOffsets[1].x = Width >> i;
		region.dstOffsets[1].y = Height >> i;
		region.dstOffsets[1].z = 1;
	}
	cmd->blitImage(m_Data->m_Image, vk::ImageLayout::eTransferSrcOptimal, m_Data->m_Image, vk::ImageLayout::eTransferDstOptimal, regions, vk::Filter::eLinear);

	// Step: 4
	srcMipBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
	srcMipBarrier.dstAccessMask = vk::AccessFlagBits::eNone;
	srcMipBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
	srcMipBarrier.newLayout = CurrentLayout;
	cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlagBits(0), {}, {}, srcMipBarrier);

	// Step: 5
	dstMipBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
	dstMipBarrier.dstAccessMask = vk::AccessFlagBits::eNone;
	dstMipBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	dstMipBarrier.newLayout = CurrentLayout;
	cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlagBits(0), {}, {}, dstMipBarrier);
}

void egx::Image2D::SetLayout(vk::ImageLayout layout)
{
	if (layout == vk::ImageLayout::eUndefined) {
		LOG(WARNING, "Cannot set image layout to undefined.");
		return;
	}
	ScopedCommandBuffer cmd(m_Data->m_Ctx);

	vk::ImageMemoryBarrier2 imageBarrier = vk::ImageMemoryBarrier2()
		.setOldLayout(CurrentLayout).setNewLayout(layout)
		.setSrcStageMask(vk::PipelineStageFlagBits2::eAllGraphics).setDstStageMask(vk::PipelineStageFlagBits2::eAllGraphics)
		.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED).setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		.setSubresourceRange(vk::ImageSubresourceRange().setAspectMask(GetFormatAspectFlags(Format))
			.setLevelCount(VK_REMAINING_MIP_LEVELS).setLayerCount(VK_REMAINING_ARRAY_LAYERS))
		.setImage(m_Data->m_Image);

	vk::DependencyInfo barrier;
	barrier.setImageMemoryBarriers(imageBarrier);
	cmd->pipelineBarrier2(barrier);

	CurrentLayout = layout;
}

vk::ImageView Image2D::CreateView(int id, int mipLevel, int mipCount, vk::ComponentMapping RGBASwizzle)
{
	if (m_Data->m_Views.contains(id)) {
		LOG(WARNING, "Cannot create view with id={} because it already exists", id);
	}
	vk::ImageSubresourceRange range;
	range.setAspectMask(GetFormatAspectFlags(Format))
		.setBaseMipLevel(mipLevel)
		.setLevelCount(mipCount)
		.setLayerCount(VK_REMAINING_ARRAY_LAYERS);
	m_Data->m_Views[id] = m_Data->m_Ctx->Device.createImageView(vk::ImageViewCreateInfo(
		{}, m_Data->m_Image, vk::ImageViewType::e2D, Format, RGBASwizzle, range
	));
	return m_Data->m_Views[id];
}

vk::ImageView Image2D::GetView(int id) const
{
	return m_Data->m_Views.at(id);
}

vk::Image Image2D::GetHandle() const
{
	return m_Data->m_Image;
}

ImTextureID Image2D::GetImGuiTextureID(vk::Sampler sampler, uint32_t viewId)
{
	if (m_Data->m_TextureID) return m_Data->m_TextureID;
	m_Data->m_TextureID = ImGui_ImplVulkan_AddTexture(sampler, GetView(viewId), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	return m_Data->m_TextureID;
}

Image2D::DataWrapper::~DataWrapper() {
	if (m_TextureID)
		ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_TextureID);

	for (auto& [id, view] : m_Views) {
		m_Ctx->Device.destroyImageView(view);
	}
	if (m_Ctx && m_Allocation) {
		vmaDestroyImage(m_Ctx->Allocator, m_Image, m_Allocation);
	}
}
