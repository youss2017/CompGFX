#pragma once
#include "cvector.h"
#include <vulkan/vulkan.h>

#define VULKAN_MEMORY_KB(x) (x * 1024)
#define VULKAN_MEMORY_MB(x) (VULKAN_MEMORY_KB(x) * 1024)
#define VULKAN_MEMORY_BYTE_TO_MB(x) ((x / 1024) / 1024)

#define VULKAN_MEMORY_MAX_ALLOCATIONS 1024
#define VULKAN_MEMORY_BUFFER_DEVICE_BLOCK_SIZE VULKAN_MEMORY_MB(128)
#define VULKAN_MEMORY_BUFFER_HOST_BLOCK_SIZE VULKAN_MEMORY_MB(32)

#define VULKAN_MEMORY_TEXTURE_DEVICE_BLOCK_SIZE VULKAN_MEMORY_MB(64)
#define VULKAN_MEMORY_TEXTURE_HOST_BLOCK_SIZE VULKAN_MEMORY_MB(32)

#ifdef __cplusplus
extern "C"
{
#endif

    struct VulkanMemorySubAllocation
    {
        VkBuffer m_buffer;
        VkImage m_image;
        uint32_t m_offset;
        uint32_t m_size;
        uint8_t m_buffer_status; // 1 is alive, 0 buffer is destroyed!
    };

    struct VulkanMemoryAllocation
    {
        uint8_t m_created_flag; // 1 == yes
        VkDeviceMemory m_memory;
        VkMemoryPropertyFlagBits m_allocation_properties;
        uint32_t m_free_offset; // offset to the next area that is not used!
        uint32_t m_size;        // total size
        void *m_map_ptr;
        int32_t m_mapcount;
        uint8_t m_fragment_flag; // 0 means no buffers were freed, 1 means at least 1 buffer was freed therefore the heap is fragmented.

        cvector_vector_type(struct VulkanMemorySubAllocation) m_allocated_buffers;
    };

    struct _VulkanMemoryContext
    {
        VkDevice m_device;
        VkPhysicalDevice m_physical_device;
        struct VulkanMemoryAllocation m_allocations[VULKAN_MEMORY_MAX_ALLOCATIONS];
        uint32_t m_defragment_threshold;
        VkAllocationCallbacks* m_allocation_callbacks;
    } typedef *VulkanMemoryContext;

    VulkanMemoryContext vulkan_memory_initalize_context(VkDevice device, VkPhysicalDevice physical_device, VkAllocationCallbacks* allocation_callbacks);
    void vulkan_memory_destroy_context(VulkanMemoryContext mem_context);

    /*
    the allocator tries to create a buffer with preferred memory properties and without the invalid memory properties,
    if it can't then it tries to allocate with the required memory properties and if that fails too, then the memory allocation fails.
    returns 1 for success and 0 for fail
    */
    int vulkan_memory_allocate_buffer(VulkanMemoryContext mem_context, VkBufferCreateInfo *pCreateInfo,
                                      VkMemoryPropertyFlags preferred_memory_properties, VkMemoryPropertyFlags invalid_memory_properties, VkMemoryPropertyFlags required_memory_properties,
                                      VkBuffer *pOutBuffer, VkDeviceMemory *pOutMemory, VkDeviceSize *pOutMemoryOffset, uint32_t *pOutAllocationIndex);

    void *vulkan_memory_mapmemory(VulkanMemoryContext mem_context, VkDeviceMemory memory, VkDeviceSize memory_offset, uint32_t allocation_index, VkDeviceSize memory_suboffest, VkDeviceSize memory_size);
    void vulkan_memory_unmapmemory(VulkanMemoryContext mem_context, VkDeviceMemory memory, int offset, int size, uint32_t allocation_index);

    void vulkan_memory_flush(VulkanMemoryContext mem_context, VkDeviceMemory memory, VkDeviceSize memory_offset, VkDeviceSize memory_size, uint32_t allocation_index);

    void vulkan_memory_free_buffer(VulkanMemoryContext mem_context, VkBuffer buffer, uint32_t allocation_index);
    void vulkan_memory_defragment_memory(VulkanMemoryContext mem_context);

    int vulkan_memory_allocate_texture2d(VulkanMemoryContext mem_context, VkImageCreateInfo *pCreateInfo,
                                         VkMemoryPropertyFlags preferred_memory_properties, VkMemoryPropertyFlags invalid_memory_properties, VkMemoryPropertyFlags required_memory_properties,
                                         VkImage *pOutImage, VkDeviceMemory *pOutMemory, VkDeviceSize *pOutMemoryOffset, uint32_t *pOutAllocationIndex);
    
    void vulkan_memory_free_texture2d(VulkanMemoryContext mem_context, VkImage image, uint32_t allocation_index);

#ifdef __cplusplus
}
#endif
