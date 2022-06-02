#pragma once
#include "../EngineBase.h"
#include "vulkan_memory_allocator.hpp"

struct GPUBuffer2
{
	vk::VkContext mContext;
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

GRAPHICS_API IBuffer2 Buffer2_Create(BufferType type, size_t size, BufferMemoryType memoryType, bool pointerUsage, bool requireCoherent, bool intermediateBuffer = false);
// pData == nullptr means initalize to 0, intermediateBuffer means will you use this as an intermediate buffer BUFFER_TYPE_TRANSFER*
GRAPHICS_API IBuffer2 Buffer2_CreatePreInitalized(BufferType type, void* pData, size_t size, BufferMemoryType memoryType, bool pointerUsage, bool requireCoherent, bool intermediateBuffer = false);
GRAPHICS_API void Buffer2_UploadData(IBuffer2 buffer, void* pData, size_t offset, size_t size);
GRAPHICS_API void* Buffer2_Map(IBuffer2 buffer);
// Makes host writes (CPU) visible to device (GPU)
GRAPHICS_API void Buffer2_Flush(IBuffer2 buffer, uint64_t offset, uint64_t size);
// Makes device writes (GPU) visible to host (CPU)
GRAPHICS_API void Buffer2_Invalidate(IBuffer2 buffer, uint64_t offset, uint64_t size);
// Make sure to flush before unmapping
GRAPHICS_API void Buffer2_Unmap(IBuffer2 buffer);
GRAPHICS_API uint64_t Buffer2_GetGPUPointer(IBuffer2 buffer);
GRAPHICS_API void Buffer2_Destroy(IBuffer2 buffer);

// DisableVerification means you do not to use verify methods
GRAPHICS_API void* Gmalloc(uint32_t size, BufferType type, bool DisableVerification);
GRAPHICS_API void GverifyReadWrite(void* ptr);
GRAPHICS_API void GverifyRead(void* ptr);
GRAPHICS_API void GverifyWrite(void* ptr);
GRAPHICS_API void Gfree(void* ptr);
GRAPHICS_API IBuffer2 Gbuffer(void* ptr);
// Must have been allocated with Gmalloc
GRAPHICS_API void* Gpointer(IBuffer2 buffer);
