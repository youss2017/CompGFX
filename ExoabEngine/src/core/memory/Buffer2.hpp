#pragma once
#include "../EngineBase.h"
#include "vulkan_memory_allocator.hpp"

struct GPUBuffer2
{
	GraphicsContext mContext;
	BufferType mType;
	size_t mSize;
	BufferMemoryType mMemoryType;
	VkAlloc::BUFFER mVkAllocBuffer;
	VkBuffer mBuffers[gFrameOverlapCount];
	bool mIsMapped = false;
	void* mMappedPtr = nullptr;
	uint64_t mGPUPointer = 0;

	const VkBuffer& operator[](uint32_t frameIndex) {
		return mBuffers[frameIndex];
	}

} typedef* IBuffer2;

IBuffer2 Buffer2_Create(BufferType type, size_t size, BufferMemoryType memoryType, bool pointerUsage, bool requireCoherent);
IBuffer2 Buffer2_CreatePreInitalized(BufferType type, void* pData, size_t size, BufferMemoryType memoryType, bool pointerUsage, bool requireCoherent);
void Buffer2_UploadData(IBuffer2 buffer, void* pData, size_t offset, size_t size);
void* Buffer2_Map(IBuffer2 buffer);
// Makes host writes (CPU) visible to device (GPU)
void Buffer2_Flush(IBuffer2 buffer, uint64_t offset, uint64_t size);
// Makes device writes (GPU) visible to host (CPU)
void Buffer2_Invalidate(IBuffer2 buffer, uint64_t offset, uint64_t size);
// Make sure to flush before unmapping
void Buffer2_Unmap(IBuffer2 buffer);
uint64_t Buffer2_GetGPUPointer(IBuffer2 buffer);
void Buffer2_Destroy(IBuffer2 buffer);

void* Gmalloc(uint32_t size, BufferType type);
void Gfree(void* ptr);
IBuffer2 Gbuffer(void* ptr);
