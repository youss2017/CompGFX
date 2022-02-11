#include "Buffers.hpp"
#include "../backend/VkGraphicsCard.hpp"
#include "../backend/GLGraphicsCard.hpp"
#include "vulkan_memory.h"

PFN_GPUBuffer_Create *GPUBuffer_Create = nullptr;
PFN_GPUBuffer_UploadData *GPUBuffer_UploadData = nullptr;
PFN_GPUBuffer_MapBuffer *GPUBuffer_MapBuffer = nullptr;
PFN_GPUBuffer_UnmapBuffer *GPUBuffer_UnmapBuffer = nullptr;
PFN_GPUBuffer_Flush *GPUBuffer_Flush = nullptr;
PFN_GPUBuffer_Destroy *GPUBuffer_Destroy = nullptr;

GPUBuffer *Vulkan_GPUBuffer_Create(GraphicsContext _context, const GPUBufferSpecification *spec)
{
	GPUBuffer *result = new GPUBuffer();
	result->m_Context = _context;
	result->m_Spec = *spec;
	result->m_ApiType = 0;
	vk::VkContext context = (vk::VkContext)_context;
	VkMemoryPropertyFlags properties = 0;
	VkMemoryPropertyFlags required_properties = 0;
	VkMemoryPropertyFlags invalid_properties = 0;
	VkBufferUsageFlags usage{};
	if (result->m_Spec.m_MemoryType == BufferMemoryType::STATIC)
	{
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		required_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		invalid_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	else if (result->m_Spec.m_MemoryType == BufferMemoryType::STREAM)
	{
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		required_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		invalid_properties = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
	}
	else if (result->m_Spec.m_MemoryType == BufferMemoryType::DYNAMIC)
	{
		properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		required_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		invalid_properties = 0;
	}
	else
	{
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
	if (result->m_Spec.m_BufferType == BufferType::VertexBuffer)
	{
		usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (result->m_Spec.m_BufferType == BufferType::IndexBuffer)
	{
		usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (result->m_Spec.m_BufferType == BufferType::UniformBuffer)
	{
		usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	if (result->m_Spec.m_BufferType == BufferType::StorageBuffer)
	{
		usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (result->m_Spec.m_BufferType == BufferType::IndirectBuffer)
	{
		usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	if (result->m_Spec.m_BufferType == BufferType::INTE_TRANSFER_SRC)
	{
		usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (result->m_Spec.m_BufferType == BufferType::INTE_TRANSFER_DST)
	{
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	if(!(usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VkBufferCreateInfo createInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size = result->m_Spec.m_Size;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = 0;

	if (!vulkan_memory_allocate_buffer((VulkanMemoryContext)context->m_memory_context, &createInfo, properties, invalid_properties, required_properties, &result->m_NativeVulkanHandle, &result->m_NativeVulkanMemory, &result->m_VulkanMemoryOffset, &result->m_VulkanAllocationIndex))
	{
		logerror("Vulkan Buffer Memory Allocation Failed!");
	}
	return result;
}

void Vulkan_GPUBuffer_UploadData(GPUBuffer *buffer, void *data, size_t offset, size_t size)
{
	auto context = ((vk::VkContext)buffer->m_Context);
	auto device = context->defaultDevice;
	auto queue = context->defaultQueue;
	if (buffer->m_Spec.m_MemoryType == BufferMemoryType::DYNAMIC || buffer->m_Spec.m_MemoryType == BufferMemoryType::STREAM)
	{
		// 1) map 2) memcpy 3) unmap
		void *map_ptr = GPUBuffer_MapBuffer(buffer, false, true);
		memcpy((uint8_t*)map_ptr + offset, data, size);
		GPUBuffer_UnmapBuffer(buffer);
		return;
	}
	VkFence fence = vk::Gfx_CreateFence(context);
	VkCommandPool pool = vk::Gfx_CreateCommandPool(context, true, false);
	VkCommandBuffer cmd = vk::Gfx_AllocCommandBuffer(context, pool, true);

	// Create intermediate buffer
	auto dynamic_spec = buffer->m_Spec;
	dynamic_spec.m_MemoryType = BufferMemoryType::DYNAMIC;
	dynamic_spec.m_BufferType = BufferType::INTE_TRANSFER_SRC;
	GPUBuffer *intermediate_buffer = GPUBuffer_Create(context, &dynamic_spec);

	// upload data to intermediate buffer
	void *map_ptr = GPUBuffer_MapBuffer(intermediate_buffer, false, true);
	memcpy(map_ptr, data, size);
	GPUBuffer_UnmapBuffer(intermediate_buffer);

	// Start command buffer and copy data from intermediate buffer to target buffer
	vk::Gfx_StartCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VkBufferCopy region{};
	region.dstOffset = offset;
	region.size = size;
	vkCmdCopyBuffer(cmd, intermediate_buffer->m_NativeVulkanHandle, buffer->m_NativeVulkanHandle, 1, &region);
	vkEndCommandBuffer(cmd);
	vk::Gfx_SubmitCmdBuffers(queue, {cmd}, {}, {}, {}, fence);
	vkcheck(vkWaitForFences(device, 1, &fence, true, 10'000'000'000)); // wait a maximum of 10 seconds
	// CleanUp
	GPUBuffer_Destroy(intermediate_buffer);

	vkDestroyFence(device, fence, context->m_allocation_callback);
	vkDestroyCommandPool(device, pool, context->m_allocation_callback);
	if (buffer->m_Spec.m_MemoryType == BufferMemoryType::STREAM)
		GPUBuffer_Flush(buffer);
}

void *Vulkan_GPUBuffer_MapBuffer(IGPUBuffer buffer, bool ReadAccess, bool WriteAccess)
{
	if (!buffer->m_IsBufferMapped) {
		void* map_ptr;
		buffer->m_IsBufferMapped = true;
		vk::VkContext context = (vk::VkContext)buffer->m_Context;
		map_ptr = vulkan_memory_mapmemory((VulkanMemoryContext)context->m_memory_context, buffer->m_NativeVulkanMemory, buffer->m_VulkanMemoryOffset, buffer->m_VulkanAllocationIndex, 0, 0);
		buffer->m_mapped_pointer = map_ptr;
		return map_ptr;
	}
	else {
		return buffer->m_mapped_pointer;
	}
}

void Vulkan_GPUBuffer_UnmapBuffer(IGPUBuffer buffer)
{
	buffer->m_IsBufferMapped = false;
	buffer->m_mapped_pointer = nullptr;
	vk::VkContext context = (vk::VkContext)buffer->m_Context;
	vulkan_memory_unmapmemory((VulkanMemoryContext)context->m_memory_context, buffer->m_NativeVulkanMemory, buffer->m_VulkanMemoryOffset, buffer->m_Spec.m_Size, buffer->m_VulkanAllocationIndex);
	vulkan_memory_flush((VulkanMemoryContext)context->m_memory_context, buffer->m_NativeVulkanMemory, buffer->m_VulkanMemoryOffset, buffer->m_Spec.m_Size, buffer->m_VulkanAllocationIndex);
}

void Vulkan_GPUBuffer_Flush(IGPUBuffer buffer)
{
	if(buffer->m_IsBufferMapped) {
		VulkanMemoryContext context = (VulkanMemoryContext)(ToVKContext(buffer->m_Context)->m_memory_context);
		vulkan_memory_flush(context, buffer->m_NativeVulkanMemory, buffer->m_VulkanMemoryOffset, buffer->m_Spec.m_Size, buffer->m_VulkanAllocationIndex);
	}
}

void Vulkan_GPUBuffer_Destroy(GPUBuffer *buffer)
{
	if (buffer->m_IsBufferMapped)
		GPUBuffer_UnmapBuffer(buffer);
	vk::VkContext context = (vk::VkContext)buffer->m_Context;
	vulkan_memory_free_buffer((VulkanMemoryContext)context->m_memory_context, buffer->m_NativeVulkanHandle, buffer->m_VulkanAllocationIndex);
	delete buffer;
}

// ======================================= OPENGL =======================================

GPUBuffer *GL_GPUBuffer_Create(GraphicsContext context, const GPUBufferSpecification *spec)
{
	GPUBuffer *result = new GPUBuffer();
	GLuint bufferID;
	glGenBuffers(1, &bufferID);
	result->m_NativeOpenGLHandle = bufferID;
	result->m_Spec = *spec;
	result->m_Context = context;
	GLenum target;
	if (result->m_Spec.m_BufferType == BufferType::VertexBuffer)
		target = GL_ARRAY_BUFFER;
	else if (result->m_Spec.m_BufferType == BufferType::IndexBuffer)
		target = GL_ELEMENT_ARRAY_BUFFER;
	else if (result->m_Spec.m_BufferType == BufferType::UniformBuffer)
		target = GL_UNIFORM_BUFFER;
	else if (result->m_Spec.m_BufferType == BufferType::StorageBuffer)
		target = GL_SHADER_STORAGE_BUFFER;
	else if (result->m_Spec.m_BufferType == BufferType::IndirectBuffer)
		target = GL_DRAW_INDIRECT_BUFFER;
	else if (result->m_Spec.m_BufferType == BufferType::INTE_TRANSFER_SRC || result->m_Spec.m_BufferType == BufferType::INTE_TRANSFER_DST)
	{
		log_error("Cannot create OpenGL buffer with Transfer SRC and Transfer DST flags, these flags are exclusively for Vulkan");
		assert(0 && "Cannot create OpenGL buffer with Transfer SRC and Transfer DST flags, these flags are exclusively for Vulkan");
	}
	else
		assert(0 && "Error in EngineMemory.cpp Engine_Internal_CreateOpenGLBuffer(...)");
	glBindBuffer(target, bufferID);
	if (result->m_Spec.m_MemoryType == BufferMemoryType::STATIC)
		glBufferData(target, result->m_Spec.m_Size, NULL, GL_STATIC_DRAW);
	else if (result->m_Spec.m_MemoryType == BufferMemoryType::STREAM)
		glBufferData(target, result->m_Spec.m_Size, NULL, GL_STREAM_DRAW);
	else if (result->m_Spec.m_MemoryType == BufferMemoryType::STREAM)
		glBufferData(target, result->m_Spec.m_Size, NULL, GL_DYNAMIC_DRAW);
	return result;
}

void GL_GPUBuffer_UploadData(GPUBuffer *buffer, void *data, size_t offset, size_t size)
{
	GLenum target;
	auto bufferType = buffer->m_Spec.m_BufferType;
	if (bufferType == BufferType::VertexBuffer)
		target = GL_ARRAY_BUFFER;
	else if (bufferType == BufferType::IndexBuffer)
		target = GL_ELEMENT_ARRAY_BUFFER;
	else if (bufferType == BufferType::UniformBuffer)
		target = GL_UNIFORM_BUFFER;
	else if (bufferType == BufferType::StorageBuffer)
		target = GL_SHADER_STORAGE_BUFFER;
	else if (bufferType == BufferType::IndirectBuffer)
		target = GL_DRAW_INDIRECT_BUFFER;
	else
		assert(0 && "Error in EngineMemory.cpp Engine_Internal_UploadOpenGLBufferData(...)");
	glBindBuffer(target, buffer->m_NativeOpenGLHandle);
	glBufferSubData(target, offset, size, data);
}

void *GL_GPUBuffer_MapBuffer(GPUBuffer *buffer, bool ReadAccess, bool WriteAccess)
{
	GLenum target;
	if (buffer->m_Spec.m_BufferType == BufferType::VertexBuffer)
		target = GL_ARRAY_BUFFER;
	else if (buffer->m_Spec.m_BufferType == BufferType::IndexBuffer)
		target = GL_ELEMENT_ARRAY_BUFFER;
	else if (buffer->m_Spec.m_BufferType == BufferType::UniformBuffer)
		target = GL_UNIFORM_BUFFER;
	else if (buffer->m_Spec.m_BufferType == BufferType::StorageBuffer)
		target = GL_SHADER_STORAGE_BUFFER;
	else if (buffer->m_Spec.m_BufferType == BufferType::IndirectBuffer)
		target = GL_DRAW_INDIRECT_BUFFER;
	else
		assert(0 && "Error in EngineMemory.cpp Engine_Internal_MapOpenGLBuffer(...)");
	glBindBuffer(target, buffer->m_NativeOpenGLHandle);
	if (ReadAccess && WriteAccess)
		return glMapBuffer(target, GL_READ_WRITE);
	else if (ReadAccess)
		return glMapBuffer(target, GL_READ_ONLY);
	else if (WriteAccess)
		return glMapBuffer(target, GL_WRITE_ONLY);
	log_error("Error Mapping OpenGL buffer", __FILE__, __LINE__);
	return NULL;
}

void GL_GPUBuffer_UnmapBuffer(GPUBuffer *buffer)
{
	GLenum target;
	if (buffer->m_Spec.m_BufferType == BufferType::VertexBuffer)
		target = GL_ARRAY_BUFFER;
	else if (buffer->m_Spec.m_BufferType == BufferType::IndexBuffer)
		target = GL_ELEMENT_ARRAY_BUFFER;
	else if (buffer->m_Spec.m_BufferType == BufferType::UniformBuffer)
		target = GL_UNIFORM_BUFFER;
	else if (buffer->m_Spec.m_BufferType == BufferType::StorageBuffer)
		target = GL_SHADER_STORAGE_BUFFER;
	else if (buffer->m_Spec.m_BufferType == BufferType::IndirectBuffer)
		target = GL_DRAW_INDIRECT_BUFFER;
	else
		assert(0 && "Error in EngineMemory.cpp Engine_Internal_UnMapOpenGLBuffer(...)");
	glBindBuffer(target, buffer->m_NativeOpenGLHandle);
	glUnmapBuffer(target);
}

void GL_GPUBuffer_Flush(IGPUBuffer buffer)
{
	GL_GPUBuffer_UnmapBuffer(buffer);
}

void GL_GPUBuffer_Destroy(GPUBuffer *buffer)
{
	if (buffer->m_IsBufferMapped)
		GPUBuffer_UnmapBuffer(buffer);
	glDeleteBuffers(1, &buffer->m_NativeOpenGLHandle);
	delete buffer;
}

void GPUBuffer_LinkFunctions(GraphicsContext context)
{
	char ApiType = *(char *)context;
	if (ApiType == 0)
	{
		GPUBuffer_Create = Vulkan_GPUBuffer_Create;
		GPUBuffer_UploadData = Vulkan_GPUBuffer_UploadData;
		GPUBuffer_MapBuffer = Vulkan_GPUBuffer_MapBuffer;
		GPUBuffer_UnmapBuffer = Vulkan_GPUBuffer_UnmapBuffer;
		GPUBuffer_Flush = Vulkan_GPUBuffer_Flush;
		GPUBuffer_Destroy = Vulkan_GPUBuffer_Destroy;
	}
	else if (ApiType == 1)
	{
		GPUBuffer_Create = GL_GPUBuffer_Create;
		GPUBuffer_UploadData = GL_GPUBuffer_UploadData;
		GPUBuffer_MapBuffer = GL_GPUBuffer_MapBuffer;
		GPUBuffer_UnmapBuffer = GL_GPUBuffer_UnmapBuffer;
		GPUBuffer_Flush = GL_GPUBuffer_Flush;
		GPUBuffer_Destroy = GL_GPUBuffer_Destroy;
	}
	else
	{
		assert(0);
		Utils::Break();
	}
}
