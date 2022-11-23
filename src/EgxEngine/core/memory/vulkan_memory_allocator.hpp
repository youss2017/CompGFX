#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
#include <map>
#include <vma/vk_mem_alloc.h>

#define VK_ALLOC_KB(x) ((unsigned long long)(x * 1024.))
#define VK_ALLOC_MB(x) ((unsigned long long)(x * 1024. * 1024.))
#define VK_ALLOC_GB(x) ((unsigned long long)(x * 1024. * 1024. * 1024.))

#define VK_ALLOC_B_TO_KB(x) ((unsigned long long)(x / (1024.)))
#define VK_ALLOC_B_TO_MB(x) ((unsigned long long)(x / (1024. * 1024.0)))
#define VK_ALLOC_B_TO_GB(x) ((unsigned long long)(x / (1024. * 1024. * 1024.)))

namespace VkAlloc
{

	enum class DEVICE_MEMORY_PROPERTY
	{
		CPU_ONLY,
		CPU_TO_GPU,
		GPU_ONLY
	};

	struct DEVICE_HEAP_SUBALLOCATION
	{
		bool m_coherent = false;
		bool m_host_visible = false;
		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocation_info;
	};

	struct BUFFER_DESCRIPTION {
		DEVICE_MEMORY_PROPERTY m_properties;
		VkBufferUsageFlags m_usage;
		size_t m_size;
		bool mRequireCoherent;
		float m_priority = 0.5f;
	};

	struct _BUFFER
	{
		VkBuffer m_buffer;
		DEVICE_MEMORY_PROPERTY m_properties;
		DEVICE_HEAP_SUBALLOCATION m_suballocation;
		BUFFER_DESCRIPTION m_description;
	} typedef *BUFFER;

	struct IMAGE_DESCRIPITION
	{
		DEVICE_MEMORY_PROPERTY   m_properties;
		VkImageCreateFlags       m_flags;
		VkImageType              m_imageType;
		VkFormat                 m_format;
		VkExtent3D               m_extent;
		uint32_t                 m_mipLevels;
		uint32_t                 m_arrayLayers;
		VkSampleCountFlagBits    m_samples;
		VkImageTiling			 m_tiling;
		VkImageUsageFlags        m_usage;
		bool m_lazyAllocate;
		float m_priority = 0.5f;
	};

	struct _IMAGE
	{
		VkImage m_image;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_depth;
		VkSampleCountFlags m_sampleCount;
		DEVICE_MEMORY_PROPERTY m_properties;
		DEVICE_HEAP_SUBALLOCATION m_suballocation;
		IMAGE_DESCRIPITION m_description;
	} typedef *IMAGE;

	struct _CONTEXT
	{
		VkDevice m_device;
		VkPhysicalDevice m_physical_device;
		uint32_t m_HeapBlockAllocationSize;
		VmaAllocator m_allocator;
	} typedef *CONTEXT;

	CONTEXT CreateContext(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, uint32_t HeapBlockAllocationSizeInBytes, bool UsingBufferReference);
	void  DestroyContext(CONTEXT context);

	VkResult CreateBuffers(CONTEXT context, uint32_t count, BUFFER_DESCRIPTION* pDescs, BUFFER* pOutBuffers);
	VkResult CreateImages(CONTEXT context, uint32_t count, IMAGE_DESCRIPITION* pDescs, IMAGE* pOutImages);

	void MapBuffer(CONTEXT context, BUFFER buffer);
	// Writes by CPU become visible to GPU (for non-coherent)
	void FlushBuffer(CONTEXT context, BUFFER buffer, size_t offset, size_t size);
	// Writes by GPU become visible to CPU (for non-coherent)
	void InvalidateBuffer(CONTEXT context, BUFFER buffer, size_t offset, size_t size);
	void UnmapBuffer(CONTEXT context, BUFFER buffer);

	void DestroyBuffers(const CONTEXT context, uint32_t count, const BUFFER* pBuffers);
	void DestroyImages(const CONTEXT context, uint32_t count, const IMAGE* pImages);

	void Defragment(CONTEXT context);

}