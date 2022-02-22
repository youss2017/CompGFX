#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
#include <map>

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
		bool m_destroyed = false;
		bool m_coherent = false;
		bool m_host_visible = false;
		uint32_t m_offset;
		uint32_t m_size;
		uint32_t m_heap_index;
		uint32_t m_suballocation_index;
		VkDeviceMemory m_memory;
		void* m_buffer = nullptr;
		void* m_image = nullptr;
		char8_t* m_mapped_pointer = nullptr;
	};

	struct DEVICE_HEAP_FREE_BLOCK
	{
		uint32_t m_offset;
		uint32_t m_size;
	};

	struct _DEVICE_HEAP
	{
		DEVICE_MEMORY_PROPERTY m_properties;
		VkDeviceMemory m_memory;
		uint32_t m_size;
		uint32_t m_map_count;
		uint32_t m_heap_index;
		uint32_t m_fragmentation_score = 0;
		char8_t* m_mapped_pointer = nullptr;
		bool m_coherent = false;
		bool m_host_visible = false;
		// To keep track of memory blocks that are freed
		// this is caused by memory fragmenation.
		// when this is created there is only 1 block
		// with the range [0, m_size]
		std::vector<DEVICE_HEAP_FREE_BLOCK> m_free_blocks;
		std::map<uint32_t, DEVICE_HEAP_SUBALLOCATION> m_suballocations;
	} typedef *DEVICE_HEAP;

	struct BUFFER_DESCRIPTION {
		DEVICE_MEMORY_PROPERTY m_properties;
		VkBufferUsageFlags m_usage;
		uint32_t m_size;
	};

	struct _BUFFER
	{
		VkBuffer m_buffer;
		DEVICE_MEMORY_PROPERTY m_properties;
		DEVICE_HEAP_SUBALLOCATION m_suballocation;
		// This the block of memory occuiped by this buffer (includes alignments)
		DEVICE_HEAP_FREE_BLOCK m_heap_free_block;
		VkMemoryRequirements m_memrequirements;
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
		VkImageTiling            m_tiling;
		VkImageUsageFlags        m_usage;
	};

	struct _IMAGE
	{
		VkImage m_image;
		uint32_t m_width;
		uint32_t m_height;
		VkSampleCountFlags m_sampleCount;
		DEVICE_MEMORY_PROPERTY m_properties;
		DEVICE_HEAP_SUBALLOCATION m_suballocation;
		VkMemoryRequirements m_memrequirements;
		IMAGE_DESCRIPITION m_description;
		DEVICE_HEAP_FREE_BLOCK m_heap_free_block;
	} typedef *IMAGE;

	struct _CONTEXT
	{
		VkDevice m_device;
		VkPhysicalDevice m_physical_device;
		uint32_t m_max_allocations;
		uint32_t m_minMemoryMapAlignment;
		uint32_t m_minTexelBufferOffsetAlignment;
		uint32_t m_minUniformBufferOffsetAlignment;
		uint32_t m_minStorageBufferOffsetAlignment;
		uint32_t m_HeapBlockAllocationSize;
		uint32_t m_next_heap_index;
		std::vector<DEVICE_HEAP> m_device_heap;
	} typedef *CONTEXT;

	CONTEXT CreateContext(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t HeapBlockAllocationSizeInBytes);
	void  DestroyContext(CONTEXT context);

	VkBool32 CreateBuffers(CONTEXT context, uint32_t count, BUFFER_DESCRIPTION* pDescs, BUFFER* pOutBuffers);
	VkBool32 CreateImages(CONTEXT context, uint32_t count, IMAGE_DESCRIPITION* pDescs, IMAGE* pOutImages);

	void MapBuffer(CONTEXT context, BUFFER buffer);
	// Writes by CPU become visible to GPU (for non-coherent)
	void FlushBuffer(CONTEXT context, BUFFER buffer);
	// Writes by GPU become visible to CPU (for non-coherent)
	void InvalidateBuffer(CONTEXT context, BUFFER buffer);
	void UnmapBuffer(CONTEXT context, BUFFER buffer);

	void DestroyBuffers(CONTEXT context, uint32_t count, BUFFER* pBuffers);
	void DestroyImages(CONTEXT context, uint32_t count, IMAGE* pImages);

	void Defragment(CONTEXT context);

}