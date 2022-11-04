#include "egxmemory.hpp"
#include "../egxutil.hpp"
#include "stb/stb_image.h"
#include "stb/stb_image_resize.h"
#include "../../cmd/cmd.hpp"
#include <imgui/backends/imgui_impl_vulkan.h>
#include <cmath>
#include <vector>
#include <Utility/CppUtility.hpp>

#pragma region Buffer
egx::ref<egx::Buffer>  egx::Buffer::FactoryCreate(
	const ref<VulkanCoreInterface>& CoreInterface,
	size_t size,
	memorylayout layout,
	buffertype type,
	bool requireCoherent)
{
#ifdef _DEBUG
	if (size > 1 * (1024 * 1024)) {
		LOG(INFO, "Creating a {0} Mb Buffer", (double)size / (1024.0 * 1024.0));
	}
#endif
	VkAlloc::DEVICE_MEMORY_PROPERTY property;
	VkFlags bufferUsage =
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	switch (layout) {
	case(memorylayout::local): {property = VkAlloc::DEVICE_MEMORY_PROPERTY::GPU_ONLY; break; }
	case(memorylayout::dynamic): {property = VkAlloc::DEVICE_MEMORY_PROPERTY::CPU_TO_GPU; break; }
	case(memorylayout::stream): {property = VkAlloc::DEVICE_MEMORY_PROPERTY::CPU_ONLY; break; }
	default: property = VkAlloc::DEVICE_MEMORY_PROPERTY::CPU_ONLY;
	}
	if (type & buffertype_vertex) bufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (type & buffertype_index) bufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (type & buffertype_uniform) bufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (type & buffertype_storage) bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	VkAlloc::BUFFER_DESCRIPTION desc{};
	desc.m_properties = property;
	desc.m_usage = bufferUsage;
	desc.m_size = size;
	desc.mRequireCoherent = requireCoherent;
	VkAlloc::BUFFER buffer;
	VkResult ret = VkAlloc::CreateBuffers(CoreInterface->MemoryContext, 1, &desc, &buffer);
	if (ret != VK_SUCCESS)
		return egx::ref<egx::Buffer>(nullptr);
	return egx::ref<egx::Buffer>(new egx::Buffer(
		size, layout,
		type, requireCoherent,
		buffer->m_buffer, CoreInterface->MemoryContext,
		buffer, CoreInterface
	));
}

egx::Buffer::~Buffer()
{
	VkAlloc::DestroyBuffers(_context, 1, &_buffer);
}

egx::Buffer::Buffer(Buffer&& move) noexcept
	:
	Size(move.Size), Layout(move.Layout),
	Type(move.Type), CoherentFlag(move.CoherentFlag),
	Buf(move.Buf), _context(move._context),
	_buffer(move._buffer)
{
	this->~Buffer();
	memset(&move, 0, sizeof(Buffer));
}

egx::Buffer& egx::Buffer::operator=(Buffer& move) noexcept
{
	this->~Buffer();
	memcpy(this, &move, sizeof(Buffer));
	memset(&move, 0, sizeof(Buffer));
	return *this;
}

egx::ref<egx::Buffer> egx::Buffer::clone()
{
	return Buffer::FactoryCreate(_coreinterface, Size, Layout, Type, CoherentFlag);
}

egx::ref<egx::Buffer> egx::Buffer::cloneAndCopy()
{
	auto clone = this->clone();
	if (!clone.IsValidRef()) return clone;
	clone->copy(this);
	return clone;
}
void  egx::Buffer::copy(ref<Buffer>& source)
{
	this->copy(source());
}

void egx::Buffer::copy(Buffer* source)
{
	assert(Size == source->Size);
	this->copy(source->Buf, 0, Size);
}

void egx::Buffer::write(void* data, size_t offset, size_t size)
{
	if (Layout != memorylayout::local) {
		bool mapState = _ptr == nullptr;
		char* ptr = static_cast<char*>(this->map());
		ptr += offset;
		memcpy(ptr, data, size);
		if (mapState) unmap();
		return;
	}
	ref<Buffer> stage = FactoryCreate(_coreinterface, size, memorylayout::stream, Type);
	stage->write(data);
	stage->flush();
	this->copy(stage->Buf, offset, size);
}

void  egx::Buffer::write(void* data, size_t size)
{
	return this->write(data, 0, size);
}

void  egx::Buffer::write(void* data)
{
	return this->write(data, 0, Size);
}

void* egx::Buffer::map()
{
	assert(Layout != memorylayout::local);
	if (_ptr) return _ptr;
	VkAlloc::MapBuffer(_context, _buffer);
	_ptr = _buffer->m_suballocation.m_allocation_info.pMappedData;
	return _ptr;
}

void  egx::Buffer::unmap()
{
	assert(Layout != memorylayout::local);
	_ptr = nullptr;
	VkAlloc::UnmapBuffer(_context, _buffer);
}

void  egx::Buffer::read(void* pOutput, size_t offset, size_t size)
{
	assert(offset + size < Size);
	if (Layout != memorylayout::local) {
		bool mapState = _ptr == nullptr;
		char* ptr = static_cast<char*>(this->map());
		ptr += offset;
		memcpy(pOutput, ptr, size);
		return;
	}
	ref<Buffer> stage = FactoryCreate(_coreinterface, size, memorylayout::stream, buffertype_onlytransfer);
	auto cmd = CommandBufferSingleUse(_coreinterface);

	{
		VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		barrier.buffer = Buf;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.srcQueueFamilyIndex =
			barrier.dstQueueFamilyIndex =
			VK_QUEUE_FAMILY_IGNORED;
		barrier.offset = offset;
		barrier.size = size;
		vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
			1, &barrier,
			0, nullptr);
	}

	{
		VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		barrier.buffer = stage->Buf;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.srcQueueFamilyIndex =
			barrier.dstQueueFamilyIndex =
			VK_QUEUE_FAMILY_IGNORED;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;
		vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
			1, &barrier,
			0, nullptr);
	}

	VkBufferMemoryBarrier barriers[2]{};
	barriers[0].sType = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barriers[0].buffer = Buf;
	barriers[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barriers[0].srcQueueFamilyIndex =
		barriers[0].dstQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;
	barriers[0].offset = offset;
	barriers[0].size = size;

	barriers[1].buffer = stage->Buf;
	barriers[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barriers[1].srcQueueFamilyIndex =
		barriers[1].dstQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;
	barriers[1].offset = 0;
	barriers[1].size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
		2, barriers,
		0, nullptr);

	VkBufferCopy region{};
	region.srcOffset = offset;
	region.size = size;
	vkCmdCopyBuffer(cmd.Cmd, this->Buf, stage->Buf, 1, &region);
	cmd.Execute();

	stage->invalidate();
	char* ptr = static_cast<char*>(stage->map());
	ptr += offset;
	memcpy(pOutput, ptr, size);
}

void  egx::Buffer::read(void* pOutput, size_t size)
{
	this->read(pOutput, 0, size);
}

void  egx::Buffer::read(void* pOutput)
{
	this->read(pOutput, 0, Size);
}

void  egx::Buffer::flush()
{
	VkAlloc::FlushBuffer(_context, _buffer, 0, Size);
}

void  egx::Buffer::flush(size_t offset, size_t size)
{
	assert(offset + size < Size);
	VkAlloc::FlushBuffer(_context, _buffer, offset, size);
}

void  egx::Buffer::invalidate()
{
	VkAlloc::InvalidateBuffer(_context, _buffer, 0, Size);
}

void  egx::Buffer::invalidate(size_t offset, size_t size)
{
	assert(offset + size < Size);
	VkAlloc::InvalidateBuffer(_context, _buffer, offset, size);
}

void  egx::Buffer::copy(VkBuffer src, size_t offset, size_t size)
{
	auto cmd = CommandBufferSingleUse(_coreinterface);

	VkBufferMemoryBarrier barriers[2]{};
	barriers[0].sType = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barriers[0].buffer = Buf;
	barriers[0].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barriers[0].srcQueueFamilyIndex =
		barriers[0].dstQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;
	barriers[0].offset = offset;
	barriers[0].size = size;

	barriers[1].sType = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barriers[1].buffer = src;
	barriers[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barriers[1].srcQueueFamilyIndex =
		barriers[1].dstQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;
	barriers[1].offset = 0;
	barriers[1].size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
		2, barriers,
		0, nullptr);

	VkBufferCopy region{};
	region.srcOffset = offset;
	region.size = size;
	vkCmdCopyBuffer(cmd.Cmd, src, this->Buf, 1, &region);

	cmd.Execute();
}
#pragma endregion

#pragma region Image
#pragma region FactoryCreate
egx::ref<egx::Image>  egx::Image::FactoryCreate(
	const ref<VulkanCoreInterface>& CoreInterface,
	memorylayout layout,
	VkImageAspectFlags aspect,
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	VkFormat format,
	uint32_t mipcount,
	uint32_t arraylevel,
	VkImageUsageFlags usage,
	VkImageLayout InitalLayout)
{
	VkAlloc::IMAGE_DESCRIPITION desc{};
	switch (layout) {
	case(memorylayout::local): {
		desc.m_properties = VkAlloc::DEVICE_MEMORY_PROPERTY::GPU_ONLY;
		break;
	}case(memorylayout::dynamic): {
		desc.m_properties = VkAlloc::DEVICE_MEMORY_PROPERTY::CPU_TO_GPU;
		break;
	}case(memorylayout::stream): {
		desc.m_properties = VkAlloc::DEVICE_MEMORY_PROPERTY::CPU_ONLY;
		break;
	}
	default:
		assert(0);
	}
	VkImageType type = VK_IMAGE_TYPE_1D;
	if (height > 1) type = VK_IMAGE_TYPE_2D;
	if (depth > 1) type = VK_IMAGE_TYPE_3D;
	desc.m_format = format;
	desc.m_imageType = type;
	desc.m_extent.width = width;
	desc.m_extent.height = height;
	desc.m_extent.depth = depth;
	uint32_t MipCount = (uint32_t)std::min(std::log2(width), std::log2(height));
	if (mipcount == 0) mipcount = MipCount;
	mipcount = std::min(mipcount, MipCount);
	desc.m_mipLevels = mipcount;
	desc.m_arrayLayers = std::max(arraylevel, 1u);
	desc.m_samples = VK_SAMPLE_COUNT_1_BIT;
	desc.m_tiling = VK_IMAGE_TILING_OPTIMAL;
	desc.m_usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkAlloc::IMAGE image;
	VkResult ret = VkAlloc::CreateImages(CoreInterface->MemoryContext, 1, &desc, &image);

	if (ret != VK_SUCCESS) return ref<Image>((Image*)nullptr);

	auto result = ref<Image>(new Image(width, height, depth, mipcount, desc.m_arrayLayers,
		image->m_image, usage, CoreInterface->MemoryContext, image, CoreInterface, format, aspect, layout, desc.m_usage, InitalLayout));
	if(InitalLayout != VK_IMAGE_LAYOUT_UNDEFINED)
		result->setlayout(VK_IMAGE_LAYOUT_UNDEFINED, InitalLayout);
	return result;
}
#pragma endregion

egx::Image::~Image() noexcept
{
	for (auto& [id, view] : _views)
		vkDestroyImageView(_coreinterface->Device, view, nullptr);
	VkAlloc::DestroyImages(_context, 1, &_image);
}

#pragma region Image write
void egx::Image::setlayout(VkImageLayout OldLayout, VkImageLayout NewLayout)
{
	auto cmd = egx::CommandBufferSingleUse(_coreinterface);
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.oldLayout = OldLayout;
	barrier.newLayout = NewLayout;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = ImageAspect;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.image = Img;
	vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
	cmd.Execute();
}

void  egx::Image::write(
	uint8_t* Data,
	VkImageLayout CurrentLayout,
	uint32_t xOffset,
	uint32_t yOffset,
	uint32_t zOffset,
	uint32_t Width,
	uint32_t Height,
	uint32_t Depth,
	uint32_t ArrayLevel,
	uint32_t StrideSizeInBytes)
{
	assert(ArrayLevel < this->Arraylevels);
	ref<Buffer> stage = Buffer::FactoryCreate(_coreinterface, Height * StrideSizeInBytes, memorylayout::stream, buffertype::buffertype_onlytransfer);
	stage->write(Data);
	stage->flush();
	auto cmd = CommandBufferSingleUse(_coreinterface);

	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_NONE;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = CurrentLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = Img;
	barrier.subresourceRange.aspectMask = ImageAspect;
	barrier.subresourceRange.baseArrayLayer = ArrayLevel;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	VkBufferImageCopy region{};
	region.imageOffset = { (int32_t)xOffset, (int32_t)yOffset, (int32_t)zOffset };
	region.imageExtent = { Width, Height, Depth };
	region.imageSubresource.aspectMask = ImageAspect;
	region.imageSubresource.baseArrayLayer = ArrayLevel;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;
	vkCmdCopyBufferToImage(cmd.Cmd, stage->Buf, Img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = CurrentLayout;
	barrier.srcAccessMask = barrier.dstAccessMask;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	cmd.Execute();
}

void  egx::Image::write(uint8_t* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t ArrayLevel, uint32_t StrideInBytes)
{
	this->write(Data, CurrentLayout, 0u, 0u, 0u, Width, Height, Depth, ArrayLevel, StrideInBytes);
}

void  egx::Image::write(uint8_t* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t Height, uint32_t ArrayLevel, uint32_t StrideInBytes)
{
	this->write(Data, CurrentLayout, 0u, 0u, 0u, Width, Height, 1u, ArrayLevel, StrideInBytes);
}

void  egx::Image::write(uint8_t* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t ArrayLevel, uint32_t StrideInBytes)
{
	this->write(Data, CurrentLayout, 0u, 0u, 0u, Width, 1u, 1u, ArrayLevel, StrideInBytes);
}
#pragma endregion

#pragma region generate mipmap
void egx::Image::generatemipmap(VkImageLayout CurrentLayout, uint32_t ArrayLevel)
{
	assert(Mipcount > 1);
	auto cmd = CommandBufferSingleUse(_coreinterface);

	// 1) First mip to transiation src
	VkImageMemoryBarrier TransitionBarrier[2]{};
	TransitionBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	TransitionBarrier[0].srcAccessMask = VK_ACCESS_2_NONE;
	TransitionBarrier[0].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
	TransitionBarrier[0].oldLayout = CurrentLayout;
	TransitionBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	TransitionBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	TransitionBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	TransitionBarrier[0].image = Img;
	TransitionBarrier[0].subresourceRange.aspectMask = ImageAspect;
	TransitionBarrier[0].subresourceRange.baseArrayLayer = ArrayLevel;
	TransitionBarrier[0].subresourceRange.baseMipLevel = 0;
	TransitionBarrier[0].subresourceRange.layerCount = 1;
	TransitionBarrier[0].subresourceRange.levelCount = 1;

	TransitionBarrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	TransitionBarrier[1].srcAccessMask = VK_ACCESS_2_NONE;
	TransitionBarrier[1].dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	TransitionBarrier[1].oldLayout = CurrentLayout;
	TransitionBarrier[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	TransitionBarrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	TransitionBarrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	TransitionBarrier[1].image = Img;
	TransitionBarrier[1].subresourceRange.aspectMask = ImageAspect;
	TransitionBarrier[1].subresourceRange.baseArrayLayer = ArrayLevel;
	TransitionBarrier[1].subresourceRange.baseMipLevel = 1;
	TransitionBarrier[1].subresourceRange.layerCount = 1;
	TransitionBarrier[1].subresourceRange.levelCount = Mipcount - 1;

	vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 0, 0, 2, TransitionBarrier);

	// 2) Blit Mips
	std::vector<VkImageBlit> regions(Mipcount - 1);
	for (int i = 1, j = 0; i <= regions.size(); i++, j++) {
		regions[j] = {};
		regions[j].srcSubresource.aspectMask = ImageAspect;
		regions[j].srcSubresource.mipLevel = 0;
		// (TODO): Generate mips for all layers
		regions[j].srcSubresource.baseArrayLayer = ArrayLevel;
		regions[j].srcSubresource.layerCount = 1;
		regions[j].srcOffsets[1].x = Width;
		regions[j].srcOffsets[1].y = Height;
		regions[j].srcOffsets[1].z = 1;
		regions[j].dstSubresource.aspectMask = ImageAspect;
		regions[j].dstSubresource.baseArrayLayer = 0;
		regions[j].dstSubresource.layerCount = 1;
		regions[j].dstSubresource.mipLevel = i;
		regions[j].dstOffsets[1].x = Width >> i;
		regions[j].dstOffsets[1].y = Height >> i;
		regions[j].dstOffsets[1].z = 1;
	}
	vkCmdBlitImage(cmd.Cmd, Img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)regions.size(), regions.data(), VK_FILTER_LINEAR);

	// 3) Transition to original layout
	VkImageMemoryBarrier OriginalBarrier[2]{};
	OriginalBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	OriginalBarrier[0].srcAccessMask = VK_ACCESS_NONE;
	OriginalBarrier[0].dstAccessMask = VK_ACCESS_NONE;
	OriginalBarrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	OriginalBarrier[0].newLayout = CurrentLayout;
	OriginalBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	OriginalBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	OriginalBarrier[0].image = Img;
	OriginalBarrier[0].subresourceRange.aspectMask = ImageAspect;
	OriginalBarrier[0].subresourceRange.baseArrayLayer = ArrayLevel;
	OriginalBarrier[0].subresourceRange.baseMipLevel = 0;
	OriginalBarrier[0].subresourceRange.layerCount = 1;
	OriginalBarrier[0].subresourceRange.levelCount = 1;

	OriginalBarrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	OriginalBarrier[1].srcAccessMask = VK_ACCESS_NONE;
	OriginalBarrier[1].dstAccessMask = VK_ACCESS_NONE;
	OriginalBarrier[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	OriginalBarrier[1].newLayout = CurrentLayout;
	OriginalBarrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	OriginalBarrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	OriginalBarrier[1].image = Img;
	OriginalBarrier[1].subresourceRange.aspectMask = ImageAspect;
	OriginalBarrier[1].subresourceRange.baseArrayLayer = ArrayLevel;
	OriginalBarrier[1].subresourceRange.baseMipLevel = 1;
	OriginalBarrier[1].subresourceRange.layerCount = 1;
	OriginalBarrier[1].subresourceRange.levelCount = Mipcount - 1;

	vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 0, 0, 2, OriginalBarrier);

	cmd.Execute();
}
#pragma endregion

egx::ref<egx::Image> EGX_API egx::Image::LoadFromDisk(ref<VulkanCoreInterface>& CoreInterface, std::string_view path, VkImageUsageFlags usage, VkImageLayout InitalLayout) {
	int x, y, c;
	auto pixels = stbi_load(path.data(), &x, &y, &c, 4);
	if (!pixels) return { (Image*)nullptr };
	auto image = egx::Image::FactoryCreate(CoreInterface, egx::memorylayout::local, VK_IMAGE_ASPECT_COLOR_BIT, x, y, 1, VK_FORMAT_R8G8B8A8_UNORM, 0, 0, usage,
		InitalLayout);
	image->write(pixels, InitalLayout, x, y, 0, 4 * x);
	image->generatemipmap(InitalLayout);
	return image;
}


#pragma region image view
VkImageView  egx::Image::createview(uint32_t ViewId, uint32_t Miplevel, uint32_t MipCount, uint32_t ArrayLevel, uint32_t ArrayCount, VkComponentMapping RGBASwizzle)
{
	assert(_views.find(ViewId) == _views.end());
	VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.image = Img;
	if (ArrayLevel <= 1) {
		if (Depth > 1)
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		else if (Height > 1)
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		else
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
	}
	else {
		if (Depth > 1)
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		else if (Height > 1)
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		else
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
	}
	createInfo.format = Format;
	createInfo.components = RGBASwizzle;
	createInfo.subresourceRange.aspectMask = ImageAspect;
	createInfo.subresourceRange.baseArrayLayer = ArrayLevel;
	createInfo.subresourceRange.layerCount = ArrayCount;
	createInfo.subresourceRange.baseMipLevel = Miplevel;
	createInfo.subresourceRange.levelCount = Mipcount;
	VkImageView view;
	vkCreateImageView(_coreinterface->Device, &createInfo, nullptr, &view);
	_views[ViewId] = view;
	return view;
}
VkImageView  egx::Image::createview(uint32_t ViewId, VkComponentMapping RGBASwizzle)
{
	return createview(ViewId, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_MIP_LEVELS, RGBASwizzle);
}
#pragma endregion

#pragma region cubemap
egx::ref<egx::Image>  egx::Image::CreateCubemap(ref<VulkanCoreInterface>& CoreInterface, std::string_view path, VkFormat format)
{
	int width, height, channels;
	struct Pixel {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
	uint32_t* pixels = (uint32_t*)stbi_load(path.data(), &width, &height, &channels, 4);
	// 1) Get Individual cube faces
	// Inverse Gamma Correction
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Pixel p = *(Pixel*)&pixels[y * width + x];
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

	uint32_t* faces[6]{};
	faces[0] = CreateFace(pixels, 2, 1);
	faces[1] = CreateFace(pixels, 0, 1);
	faces[2] = CreateFace(pixels, 1, 0);
	faces[3] = CreateFace(pixels, 1, 2);
	faces[4] = CreateFace(pixels, 1, 1);
	faces[5] = CreateFace(pixels, 3, 1);

	// 2) Create mipmaps
	auto CreateMips = [face_size](uint32_t* face, uint32_t mipindex) throw() -> uint32_t* {
		int mipcount = (int)std::log2(face_size);
		int size = face_size >> mipindex;
		uint32_t* mip = new uint32_t[size * size];
		stbir_resize_uint8((uint8_t*)face, face_size, face_size, face_size * sizeof(uint32_t), (uint8_t*)mip, size, size, size * 4, 4);
		return mip;
	};

	ref<Image> cubeFaces[6];
	for (int i = 0; i < 6; i++) {
		cubeFaces[i] = Image::FactoryCreate(
			CoreInterface,
			memorylayout::stream,
			VK_IMAGE_ASPECT_COLOR_BIT,
			face_size,
			face_size,
			1,
			format,
			0,
			1,
			0,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
		assert(cubeFaces[i].IsValidRef());
		cubeFaces[i]->generatemipmap(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	ref<Image> cubeMap = Image::FactoryCreate(
		CoreInterface,
		memorylayout::local,
		VK_IMAGE_ASPECT_COLOR_BIT,
		face_size,
		face_size,
		1,
		format,
		0,
		1,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	auto cmd = CommandBufferSingleUse(CoreInterface);

	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = cubeMap->Img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vkCmdPipelineBarrier(cmd.Cmd,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, nullptr, 0, nullptr,
		1, &barrier);

	for (int i = 0; i < 6; i++) {
		barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = cubeFaces[i]->Img;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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
			region.extent = { uint32_t(face_size >> j), uint32_t(face_size >> j), 1 };
			vkCmdCopyImage(cmd.Cmd, cubeFaces[i]->Img,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				cubeMap->Img,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}
	}

	barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = cubeMap->Img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vkCmdPipelineBarrier(cmd.Cmd,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr,
		1, &barrier);

	cmd.Execute();
	stbi_image_free(pixels);
	return cubeMap;
}

#pragma endregion

void egx::Image::barrier(VkCommandBuffer cmd, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
	VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlags srcAccess, VkAccessFlags dstAccess,
	uint32_t miplevel,
	uint32_t arraylevel,
	uint32_t mipcount,
	uint32_t arraycount) const {
	auto barr = barrier(oldLayout, newLayout, srcAccess, dstAccess, miplevel, arraylevel, mipcount, arraycount);
	vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, 0, 0, 0, 1, &barr);
}

VkImageMemoryBarrier egx::Image::barrier(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
	uint32_t miplevel,
	uint32_t arraylevel,
	uint32_t mipcount,
	uint32_t arraycount) const {
	VkImageMemoryBarrier barr{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barr.srcAccessMask = srcAccess;
	barr.dstAccessMask = dstAccess;
	barr.oldLayout = oldLayout;
	barr.newLayout = newLayout;
	barr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barr.image = Img;
	barr.subresourceRange.aspectMask = ImageAspect;
	barr.subresourceRange.baseMipLevel = miplevel;
	barr.subresourceRange.levelCount = mipcount;
	barr.subresourceRange.baseArrayLayer = arraylevel;
	barr.subresourceRange.layerCount = arraycount;
	return barr;
}

void egx::Image::read(void* buffer, VkOffset3D offset, VkExtent3D size)
{
}

egx::ref<egx::Image> egx::Image::copy(VkImageLayout CurrentLayout, VkImageLayout CopyFinalLayout) const
{
	if (CopyFinalLayout == VK_IMAGE_LAYOUT_UNDEFINED) CopyFinalLayout = CurrentLayout;
	auto result = clone();
	auto cmd = CommandBufferSingleUse(_coreinterface);
	std::vector<VkImageCopy> regions(this->Mipcount);
	for (int i = 0; i < regions.size(); i++) {
		regions[i] = {};
		regions[i].dstSubresource.aspectMask = ImageAspect;
		regions[i].dstSubresource.baseArrayLayer = 0;
		regions[i].dstSubresource.layerCount = Arraylevels;
		regions[i].dstSubresource.mipLevel = i;
		regions[i].dstSubresource = regions[i].srcSubresource;
		regions[i].extent = { Width, Height, Depth };
	}
	this->barrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, CurrentLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_TRANSFER_READ_BIT);
	result->barrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT);
	vkCmdCopyImage(cmd.Cmd, Img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, result->Img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)regions.size(), regions.data());
	this->barrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, CurrentLayout,
		VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_NONE);
	result->barrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, CopyFinalLayout,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE);
	cmd.Execute();
	return result;
}

egx::ref<egx::Image> egx::Image::clone() const
{
	return FactoryCreate(_coreinterface, _memorylayout, ImageAspect, Width, Height, Depth, Format, Mipcount, Arraylevels, _imageusage, _initallayout);
}

ImTextureID egx::Image::GetImGuiTextureID(VkSampler sampler, uint32_t viewId)
{
	if (_imgui_textureid) return _imgui_textureid;
	_imgui_textureid = ImGui_ImplVulkan_AddTexture(sampler, view(viewId), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	return _imgui_textureid;
}

#pragma endregion
