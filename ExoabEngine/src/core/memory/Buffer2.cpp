#include "Buffer2.hpp"
#include <backend/VkGraphicsCard.hpp>

IBuffer2 Buffer2_Create(GraphicsContext _context, BufferType type, size_t size, BufferMemoryType memoryType)
{
	vk::VkContext context = ToVKContext(_context);
	using namespace VkAlloc;
	BUFFER_DESCRIPTION desc;
	desc.m_properties = memoryType == BufferMemoryType::GPU_ONLY ? DEVICE_MEMORY_PROPERTY::GPU_ONLY : ((memoryType == BufferMemoryType::CPU_TO_CPU) ? DEVICE_MEMORY_PROPERTY::CPU_TO_GPU : DEVICE_MEMORY_PROPERTY::CPU_ONLY);
	desc.m_size = size;
	desc.m_usage = memoryType == BufferMemoryType::GPU_ONLY ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
	if (type & BufferType::VertexBuffer)
		desc.m_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type & BufferType::IndexBuffer)
		desc.m_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type & BufferType::UniformBuffer)
		desc.m_usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (type & BufferType::StorageBuffer)
		desc.m_usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type & BufferType::IndirectBuffer)
		desc.m_usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type & BufferType::INTE_TRANSFER_SRC)
		desc.m_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (type & BufferType::INTE_TRANSFER_DST)
		desc.m_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	IBuffer2 buffer = new GPUBuffer2();
	buffer->m_context = _context;
	buffer->memoryType = memoryType;
	buffer->size = size;
	buffer->type = type;
	VkAlloc::CreateBuffers(context->m_future_memory_context, 1, &desc, &buffer->m_vk_buffer);
	return buffer;
}

void Buffer2_UploadData(IBuffer2 buffer, char8_t *pData, size_t offset, size_t size)
{
	if (buffer->memoryType != BufferMemoryType::GPU_ONLY)
	{
		char8_t *ptr = Buffer2_Map(buffer);
		memcpy(ptr, pData, size);
		Buffer2_Unmap(buffer);
		return;
	}
	IBuffer2 intermediate = Buffer2_Create(buffer->m_context, BufferType::INTE_TRANSFER_SRC, size, BufferMemoryType::CPU_ONLY);
	Buffer2_UploadData(intermediate, pData, 0, size);
	VkFence fence = vk::Gfx_CreateFence(ToVKContext(buffer->m_context), false);
	VkCommandPool pool = vk::Gfx_CreateCommandPool(ToVKContext(buffer->m_context), true, false);
	VkCommandBuffer cmd = vk::Gfx_AllocCommandBuffer(ToVKContext(buffer->m_context), pool, true);
	vk::Gfx_StartCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VkBufferCopy region;
	region.srcOffset = 0;
	region.size = size;
	region.dstOffset = offset;
	vkCmdCopyBuffer(cmd, intermediate->m_vk_buffer->m_buffer, buffer->m_vk_buffer->m_buffer, 1, &region);
	vkEndCommandBuffer(cmd);
	vk::Gfx_SubmitCmdBuffers(ToVKContext(buffer->m_context)->defaultQueue, {cmd}, {}, {}, {}, fence);
	vkWaitForFences(ToVKContext(buffer->m_context)->defaultDevice, 1, &fence, true, 5e9);
	Buffer2_Destroy(intermediate);
	vkDestroyFence(ToVKContext(buffer->m_context)->defaultDevice, fence, ToVKContext(buffer->m_context)->m_allocation_callback);
	vkDestroyCommandPool(ToVKContext(buffer->m_context)->defaultDevice, pool, ToVKContext(buffer->m_context)->m_allocation_callback);
}

char8_t *Buffer2_Map(IBuffer2 buffer)
{
	assert(buffer->memoryType != BufferMemoryType::GPU_ONLY && "Cannot be static.");
	char8_t **mapped_pointer = (char8_t**)&buffer->m_vk_buffer->m_suballocation.m_allocation_info.pMappedData;
	if (*mapped_pointer)
		return *mapped_pointer;
	VkAlloc::MapBuffer(ToVKContext(buffer->m_context)->m_future_memory_context, buffer->m_vk_buffer);
	buffer->m_mapped = true;
	return *mapped_pointer;
}

void Buffer2_Flush(IBuffer2 buffer, uint64_t offset, uint64_t size)
{
	if (size == VK_WHOLE_SIZE)
	{
		size = buffer->size - offset;
	}
	VkAlloc::FlushBuffer(ToVKContext(buffer->m_context)->m_future_memory_context, buffer->m_vk_buffer, offset, size);
}

void Buffer2_Invalidate(IBuffer2 buffer, uint64_t offset, uint64_t size)
{
	if (size == VK_WHOLE_SIZE)
	{
		size = buffer->size - offset;
	}
	VkAlloc::InvalidateBuffer(ToVKContext(buffer->m_context)->m_future_memory_context, buffer->m_vk_buffer, offset, size);
}

void Buffer2_Unmap(IBuffer2 buffer)
{
	assert(buffer->memoryType != BufferMemoryType::GPU_ONLY && "Cannot be static.");
	buffer->m_mapped = false;
	VkAlloc::UnmapBuffer(ToVKContext(buffer->m_context)->m_future_memory_context, buffer->m_vk_buffer);
}

void Buffer2_ReAlloc(IBuffer2 buffer, size_t new_size)
{
	assert(0);// VkAlloc::ReAllocBuffer(ToVKContext(buffer->m_context)->m_future_memory_context, buffer->m_vk_buffer, new_size);
}

void Buffer2_Destroy(IBuffer2 buffer)
{
	if (buffer->m_mapped)
		Buffer2_Unmap(buffer);
	VkAlloc::DestroyBuffers(ToVKContext(buffer->m_context)->m_future_memory_context, 1, &buffer->m_vk_buffer);
}
