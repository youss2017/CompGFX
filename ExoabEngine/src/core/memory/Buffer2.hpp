#pragma once
#include "../EngineBase.h"
#include "vulkan_memory_allocator.hpp"

struct GPUBuffer2
{
	GraphicsContext m_context;
	BufferType type;
	size_t size;
	BufferMemoryType memoryType;
	VkAlloc::BUFFER m_vk_buffer;
	bool m_mapped = false;
} typedef* IBuffer2;

IBuffer2 Buffer2_Create(GraphicsContext context, BufferType type, size_t size, BufferMemoryType memoryType);
void Buffer2_UploadData(IBuffer2 buffer, char8_t* pData, size_t offset, size_t size);
char8_t* Buffer2_Map(IBuffer2 buffer);
// Makes host writes (CPU) visible to device (GPU)
void Buffer2_Flush(IBuffer2 buffer, int offset, int size);
// Makes device writes (GPU) visible to host (CPU)
void Buffer2_Invalidate(IBuffer2 buffer, int offset, int size);
// Make sure to flush before unmapping
void Buffer2_Unmap(IBuffer2 buffer);
void Buffer2_ReAlloc(IBuffer2 buffer, size_t new_size);
void Buffer2_Destroy(IBuffer2 buffer);
