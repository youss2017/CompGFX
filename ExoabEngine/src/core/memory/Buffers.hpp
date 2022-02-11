#pragma once
#include "../EngineBase.h"

struct GPUBufferSpecification
{
	void Initalize(BufferType type, size_t size, BufferMemoryType memoryType)
	{
		m_BufferType = type, m_Size = size, m_MemoryType = memoryType;
	}

	static GPUBufferSpecification SInitalize(BufferType type, size_t size, BufferMemoryType memoryType)
	{
		GPUBufferSpecification specification;
		specification.Initalize(type, size, memoryType);
		return specification;
	}

	BufferType m_BufferType;
	size_t m_Size;
	BufferMemoryType m_MemoryType;
};

struct GPUBuffer
{
	GPUBufferSpecification m_Spec;
	int m_ApiType;
	void* m_Context = nullptr;
	VkBuffer m_NativeVulkanHandle = nullptr;
	VkDeviceMemory m_NativeVulkanMemory = nullptr;
	VkDeviceSize m_VulkanMemoryOffset = 0;
	uint32_t m_VulkanAllocationIndex = 0;
	uint32_t m_NativeOpenGLHandle = 0;
	bool m_IsBufferMapped = false;
	void* m_mapped_pointer = nullptr;
};

typedef GPUBuffer* IGPUBuffer;

typedef IGPUBuffer PFN_GPUBuffer_Create(GraphicsContext context, const GPUBufferSpecification* spec);
typedef void PFN_GPUBuffer_UploadData(IGPUBuffer buffer, void* data, size_t offset, size_t size);
typedef void* PFN_GPUBuffer_MapBuffer(IGPUBuffer buffer, bool ReadAccess, bool WriteAccess);
typedef void PFN_GPUBuffer_UnmapBuffer(IGPUBuffer buffer);
typedef void PFN_GPUBuffer_Flush(IGPUBuffer buffer);
typedef void PFN_GPUBuffer_Destroy(IGPUBuffer buffer);

extern PFN_GPUBuffer_Create* GPUBuffer_Create;
extern PFN_GPUBuffer_UploadData* GPUBuffer_UploadData;
extern PFN_GPUBuffer_MapBuffer* GPUBuffer_MapBuffer;
extern PFN_GPUBuffer_UnmapBuffer* GPUBuffer_UnmapBuffer;
extern PFN_GPUBuffer_Flush* GPUBuffer_Flush;
extern PFN_GPUBuffer_Destroy* GPUBuffer_Destroy;

void GPUBuffer_LinkFunctions(GraphicsContext context);