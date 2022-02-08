#include "vulkan_memory.h"
#include "../backend/VulkanLoader.h"
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

// after VULKAN_MEMORY_DEFRAGMENT_THRESHOLD buffers are freed, then vulkan memory allocater will attempt to defragment memory heaps!
#define VULKAN_MEMORY_DEFRAGMENT_THRESHOLD 50

// Useful resource on vulkan memory types
// https://stackoverflow.com/questions/56594684/transferring-memory-from-gpu-to-cpu-with-vulkan-and-vkinvalidatemappedmemoryrang

static int vulkan_memory_internal_allocate_memory(VulkanMemoryContext mem_context, VkMemoryPropertyFlags preferred_memory_properties, VkMemoryPropertyFlags required_memory_properties,
                                                  VkMemoryRequirements memRequirement, uint32_t size, uint8_t IsBufferOrImage, uint32_t *pOutAllocationIndex);

VulkanMemoryContext vulkan_memory_initalize_context(VkDevice device, VkPhysicalDevice physical_device, VkAllocationCallbacks *allocation_callbacks)
{
    assert(device && physical_device);
    VulkanMemoryContext context = (VulkanMemoryContext)malloc(sizeof(struct _VulkanMemoryContext));
    assert(context);
    memset(context, 0, sizeof(struct _VulkanMemoryContext));
    context->m_device = device;
    context->m_physical_device = physical_device;
    context->m_allocation_callbacks = allocation_callbacks;
    return context;
}

void vulkan_memory_destroy_context(VulkanMemoryContext mem_context)
{
    vkDeviceWaitIdle(mem_context->m_device);
    for (int i = 0; i < VULKAN_MEMORY_MAX_ALLOCATIONS; i++)
    {
        struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[i];
        if (!alloc->m_created_flag)
            continue;
        for (size_t k = 0; k < cvector_size(alloc->m_allocated_buffers); k++)
            if (alloc->m_allocated_buffers[k].m_buffer_status == 1) // 1 means the buffer is alive
            {
                printf("vulkan memory allocator is freeing buffer 0x%p\n", alloc->m_allocated_buffers[k].m_buffer);
                vkDestroyBuffer(mem_context->m_device, alloc->m_allocated_buffers[k].m_buffer, mem_context->m_allocation_callbacks);
            }
        vkFreeMemory(mem_context->m_device, alloc->m_memory, mem_context->m_allocation_callbacks);
        cvector_free(alloc->m_allocated_buffers);
    }
    free(mem_context);
}

uint32_t vulkan_memory_find_heap_index(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties, uint32_t *pOutHeapIndex)
{
    assert(pOutHeapIndex);
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            *pOutHeapIndex = i;
            return 1;
        }
    }
    return 0;
}

int vulkan_memory_allocate_buffer(VulkanMemoryContext mem_context, VkBufferCreateInfo *createInfo,
                                  VkMemoryPropertyFlags preferred_memory_properties, VkMemoryPropertyFlags invalid_memory_properties,
                                  VkMemoryPropertyFlags required_memory_properties,
                                  VkBuffer *pOutBuffer, VkDeviceMemory *pOutMemory, VkDeviceSize *pOutMemoryOffset, uint32_t *pOutAllocationIndex)
{
    assert(createInfo && mem_context && pOutBuffer && pOutMemory && pOutMemoryOffset && pOutAllocationIndex);
    assert(createInfo->sharingMode == VK_SHARING_MODE_EXCLUSIVE); // No sharing memory between queues
    VkBuffer buffer;
    vkCreateBuffer(mem_context->m_device, createInfo, mem_context->m_allocation_callbacks, &buffer);
    assert(buffer);

    VkMemoryRequirements memRequirement;
    vkGetBufferMemoryRequirements(mem_context->m_device, buffer, &memRequirement);

    // Determine memory allocations that can be used!
    cvector_vector_type(int) valid_preferred_allocations = NULL;
    cvector_vector_type(int) valid_required_allocations = NULL;
    for (int i = 0; i < VULKAN_MEMORY_MAX_ALLOCATIONS; i++)
    {
        struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[i];
        if (alloc->m_created_flag)
        {
            if (alloc->m_allocation_properties & preferred_memory_properties && !(alloc->m_allocation_properties & invalid_memory_properties))
            {
                cvector_push_back(valid_preferred_allocations, i);
            }
            else if (alloc->m_allocation_properties & required_memory_properties)
            {
                cvector_push_back(valid_required_allocations, i);
            }
        }
    }

    // account for alignment
    uint32_t buffer_size = memRequirement.size;
    if (buffer_size % memRequirement.alignment != 0)
        buffer_size = (buffer_size - (buffer_size % memRequirement.alignment)) + memRequirement.alignment;

    // choose memory allocation
    // first search through preferred_allocations
    int AllocationIndex = -1;
    for (int i = 0; i < cvector_size(valid_preferred_allocations); i++)
    {
        struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[valid_preferred_allocations[i]];
        if (alloc->m_size - alloc->m_free_offset > buffer_size)
        {
            AllocationIndex = valid_preferred_allocations[i];
            goto bind_memory;
        }
    }

    // if no preferred allocations are big enough, then search through the required ones
    for (int i = 0; i < cvector_size(valid_required_allocations); i++)
    {
        struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[valid_required_allocations[i]];
        if (alloc->m_size - alloc->m_free_offset > buffer_size)
        {
            AllocationIndex = valid_required_allocations[i];
            goto bind_memory;
        }
    }

    // create new memory allocation
    if (!vulkan_memory_internal_allocate_memory(mem_context, preferred_memory_properties, required_memory_properties, memRequirement, buffer_size, 0, &AllocationIndex))
        return 0;

bind_memory:
{
    struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[AllocationIndex];
    int start_offset = (alloc->m_free_offset / memRequirement.alignment) * memRequirement.alignment;
    start_offset = (alloc->m_free_offset % memRequirement.alignment) == 0 ? start_offset : (start_offset + memRequirement.alignment);
    int end_offset = start_offset + buffer_size;
    vkBindBufferMemory(mem_context->m_device, buffer, alloc->m_memory, start_offset);
    alloc->m_free_offset = end_offset;

    struct VulkanMemorySubAllocation buffer_alloc_info;
    buffer_alloc_info.m_buffer = buffer;
    buffer_alloc_info.m_image = 0;
    buffer_alloc_info.m_offset = start_offset;
    buffer_alloc_info.m_size = buffer_size;
    buffer_alloc_info.m_buffer_status = 1;

    cvector_push_back(alloc->m_allocated_buffers, buffer_alloc_info);

    *pOutBuffer = buffer;
    *pOutMemory = alloc->m_memory;
    *pOutMemoryOffset = start_offset;
    *pOutAllocationIndex = AllocationIndex;
}

    cvector_free(valid_preferred_allocations);
    cvector_free(valid_required_allocations);

    return 1;
}

void *vulkan_memory_mapmemory(VulkanMemoryContext mem_context, VkDeviceMemory memory, VkDeviceSize memory_offset, uint32_t allocation_index, VkDeviceSize memory_suboffest, VkDeviceSize memory_size)
{
    assert(allocation_index < VULKAN_MEMORY_MAX_ALLOCATIONS);
    struct VulkanMemoryAllocation* alloc = &mem_context->m_allocations[allocation_index];
    if (memory_suboffest == 0 && memory_size == 0) {
        if (alloc->m_map_ptr)
        {
            alloc->m_mapcount++;
            return (char*)alloc->m_map_ptr + memory_offset;
        }
        assert(alloc->m_allocation_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && "Cannot map memory that is not host visible!");
        vkMapMemory(mem_context->m_device, memory, 0, VK_WHOLE_SIZE, 0, &alloc->m_map_ptr);
        alloc->m_mapcount = 1;
    }
    else {
        assert(0 && "Implement me!");
    }
    return (char *)alloc->m_map_ptr + memory_offset;
}

void vulkan_memory_unmapmemory(VulkanMemoryContext mem_context, VkDeviceMemory memory, int offset, int size, uint32_t allocation_index)
{
    struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[allocation_index];
    if (alloc->m_map_ptr)
    {
        alloc->m_mapcount--;
        if (alloc->m_mapcount <= 0)
        {
            alloc->m_map_ptr = 0;
            alloc->m_mapcount = 0;
            vkUnmapMemory(mem_context->m_device, memory);
        } else {
            vulkan_memory_flush(mem_context, memory, offset, size, allocation_index);
        }
    }
    else
    {
        printf("Vulkan Allocator Warning! Cannot unmapmemory that is not mapped!\n\a");
    }
}

void vulkan_memory_flush(VulkanMemoryContext mem_context, VkDeviceMemory memory, VkDeviceSize memory_offset, VkDeviceSize memory_size, uint32_t allocation_index)
{
    assert(allocation_index < VULKAN_MEMORY_MAX_ALLOCATIONS && mem_context && memory);
    struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[allocation_index];
    assert(alloc->m_memory == memory && "The memory passed in vulkan_memory_flush(...) is not the same that the allocation has.");
#if defined(_DEBUG)
    if (alloc->m_allocation_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
#endif
        if (!(alloc->m_allocation_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            VkMappedMemoryRange range;
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.pNext = 0;
            range.offset = memory_offset;
            range.size = memory_size;
            range.memory = memory;
            if(alloc->m_allocation_properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
                vkInvalidateMappedMemoryRanges(mem_context->m_device, 1, &range);
            vkFlushMappedMemoryRanges(mem_context->m_device, 1, &range);
        }
#if defined(_DEBUG)
    }
    else
    {
        printf("Vulkan Memory Allocator cannot flush memory that is not host visible!\n\a");
    }
#endif
}

void vulkan_memory_free_buffer(VulkanMemoryContext mem_context, VkBuffer buffer, uint32_t allocation_index)
{
    assert(allocation_index < VULKAN_MEMORY_MAX_ALLOCATIONS && mem_context && buffer);
    struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[allocation_index];
    assert(alloc->m_created_flag);
    for (int i = 0; i < cvector_size(alloc->m_allocated_buffers); i++)
    {
        struct VulkanMemorySubAllocation *buffer_alloc = &alloc->m_allocated_buffers[i];
        if (buffer == buffer_alloc->m_buffer)
        {
            vkDestroyBuffer(mem_context->m_device, buffer, mem_context->m_allocation_callbacks);
            buffer_alloc->m_buffer_status = 0;
            if (i != cvector_size(alloc->m_allocated_buffers) - 1) // its the the last buffer, then it is not fragmented
                alloc->m_fragment_flag = 1;
            else
            {
                alloc->m_free_offset -= buffer_alloc->m_size;
            }
            buffer_alloc->m_buffer = NULL;
            break;
        }
    }
    mem_context->m_defragment_threshold++;
    if (mem_context->m_defragment_threshold >= VULKAN_MEMORY_DEFRAGMENT_THRESHOLD)
    {
        vulkan_memory_defragment_memory(mem_context);
    }
}

int vulkan_memory_allocate_texture2d(VulkanMemoryContext mem_context, VkImageCreateInfo *createInfo,
                                     VkMemoryPropertyFlags preferred_memory_properties, VkMemoryPropertyFlags invalid_memory_properties, VkMemoryPropertyFlags required_memory_properties,
                                     VkImage *pOutImage, VkDeviceMemory *pOutMemory, VkDeviceSize *pOutMemoryOffset, uint32_t *pOutAllocationIndex)
{
    assert(createInfo && mem_context && pOutImage && pOutMemory && pOutMemoryOffset && pOutAllocationIndex);
    assert(createInfo->sharingMode == VK_SHARING_MODE_EXCLUSIVE); // No sharing memory between queues
    VkImage image;
    vkCreateImage(mem_context->m_device, createInfo, mem_context->m_allocation_callbacks, &image);
    assert(image);

    VkMemoryRequirements memRequirement;
    vkGetImageMemoryRequirements(mem_context->m_device, image, &memRequirement);

    // Determine memory allocations that can be used!
    cvector_vector_type(int) valid_preferred_allocations = NULL;
    cvector_vector_type(int) valid_required_allocations = NULL;
    for (int i = 0; i < VULKAN_MEMORY_MAX_ALLOCATIONS; i++)
    {
        struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[i];
        if (alloc->m_created_flag)
        {
            if (alloc->m_allocation_properties & preferred_memory_properties && !(alloc->m_allocation_properties & invalid_memory_properties))
            {
                cvector_push_back(valid_preferred_allocations, i);
            }
            else if (alloc->m_allocation_properties & required_memory_properties)
            {
                cvector_push_back(valid_required_allocations, i);
            }
        }
    }

    for (int i = 0; i < VULKAN_MEMORY_MAX_ALLOCATIONS; i++)
    {
        struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[i];
        if (alloc->m_created_flag)
        {
            if (alloc->m_allocation_properties & preferred_memory_properties && !(alloc->m_allocation_properties & invalid_memory_properties))
            {
                cvector_push_back(valid_preferred_allocations, i);
            }
            else if (alloc->m_allocation_properties & required_memory_properties)
            {
                cvector_push_back(valid_required_allocations, i);
            }
        }
    }

    // account for alignment
    uint32_t image_size = memRequirement.size;
    if (image_size % memRequirement.alignment != 0)
        image_size = (image_size - (image_size % memRequirement.alignment)) + memRequirement.alignment;

    // choose memory allocation
    // first search through preferred_allocations
    int AllocationIndex = -1;
    for (int i = 0; i < cvector_size(valid_preferred_allocations); i++)
    {
        struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[valid_preferred_allocations[i]];
        if (alloc->m_size - alloc->m_free_offset > image_size)
        {
            AllocationIndex = valid_preferred_allocations[i];
            goto bind_image_memory;
        }
    }

    // if no preferred allocations are big enough, then search through the required ones
    for (int i = 0; i < cvector_size(valid_required_allocations); i++)
    {
        struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[valid_required_allocations[i]];
        if (alloc->m_size - alloc->m_free_offset > image_size)
        {
            AllocationIndex = valid_required_allocations[i];
            goto bind_image_memory;
        }
    }

    // create new memory allocation
    if (!vulkan_memory_internal_allocate_memory(mem_context, preferred_memory_properties, required_memory_properties, memRequirement, image_size, 1, &AllocationIndex))
        return 0;

bind_image_memory:
{
    struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[AllocationIndex];
    int start_offset = (alloc->m_free_offset / memRequirement.alignment) * memRequirement.alignment;
    start_offset = (alloc->m_free_offset % memRequirement.alignment) == 0 ? start_offset : (start_offset + memRequirement.alignment);
    int end_offset = start_offset + image_size;
    vkBindImageMemory(mem_context->m_device, image, alloc->m_memory, start_offset);
    alloc->m_free_offset = end_offset;

    struct VulkanMemorySubAllocation buffer_alloc_info;
    buffer_alloc_info.m_buffer = 0;
    buffer_alloc_info.m_image = image;
    buffer_alloc_info.m_offset = start_offset;
    buffer_alloc_info.m_size = image_size;
    buffer_alloc_info.m_buffer_status = 1;

    cvector_push_back(alloc->m_allocated_buffers, buffer_alloc_info);

    *pOutImage = image;
    *pOutMemory = alloc->m_memory;
    *pOutMemoryOffset = start_offset;
    *pOutAllocationIndex = AllocationIndex;
}

    cvector_free(valid_preferred_allocations);
    cvector_free(valid_required_allocations);

    return 1;
}

static int vulkan_memory_internal_allocate_memory(VulkanMemoryContext mem_context, VkMemoryPropertyFlags preferred_memory_properties, VkMemoryPropertyFlags required_memory_properties,
                                                  VkMemoryRequirements memRequirement, uint32_t size, uint8_t IsBufferOrImage, uint32_t *pOutAllocationIndex)
{
    // find new allocation index
    int32_t new_allocation_index = -1;
    for (int i = 0; i < VULKAN_MEMORY_MAX_ALLOCATIONS; i++)
    {
        if (mem_context->m_allocations[i].m_created_flag == 0)
        {
            new_allocation_index = i;
            break;
        }
    }
    assert(new_allocation_index != -1 && "Exceeded maximum memory allocation!");
    if (pOutAllocationIndex)
        *pOutAllocationIndex = new_allocation_index;

    struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[new_allocation_index];
    alloc->m_created_flag = 1;

    uint32_t HeapIndex;
    VkMemoryPropertyFlagBits allocation_memory_properties;
    if (!vulkan_memory_find_heap_index(memRequirement.memoryTypeBits, mem_context->m_physical_device, preferred_memory_properties, &HeapIndex))
    {
        // could not get preferred heap, get requried one!
        if (!vulkan_memory_find_heap_index(memRequirement.memoryTypeBits, mem_context->m_physical_device, required_memory_properties, &HeapIndex))
        {
            // no heap could be found, allocation failed!
            printf("VULKAN MEMORY ALLOCATION ERROR! Could not find a heap with either preferred/required memory properties therefore buffer memory allocation failed!");
            alloc->m_created_flag = alloc->m_size = alloc->m_free_offset = 0;
            return 0;
        }
        else
            allocation_memory_properties = required_memory_properties;
    }
    else
        allocation_memory_properties = preferred_memory_properties;

    // TODO: Make sure not allocate more memory than exists on host!
    int allocate_size;
    if (IsBufferOrImage == 0)
    {
        if (allocation_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            allocate_size = (size > VULKAN_MEMORY_BUFFER_HOST_BLOCK_SIZE) ? size : VULKAN_MEMORY_BUFFER_HOST_BLOCK_SIZE;
        else
            allocate_size = (size > VULKAN_MEMORY_BUFFER_DEVICE_BLOCK_SIZE) ? size : VULKAN_MEMORY_BUFFER_DEVICE_BLOCK_SIZE;
    }
    else
    {
        if (allocation_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            allocate_size = (size > VULKAN_MEMORY_TEXTURE_HOST_BLOCK_SIZE) ? size : VULKAN_MEMORY_TEXTURE_HOST_BLOCK_SIZE;
        else
            allocate_size = (size > VULKAN_MEMORY_TEXTURE_DEVICE_BLOCK_SIZE) ? size : VULKAN_MEMORY_TEXTURE_DEVICE_BLOCK_SIZE;
    }

    alloc->m_free_offset = 0;
    alloc->m_size = allocate_size;

    VkMemoryAllocateInfo allocateInfo;
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = 0;
    allocateInfo.memoryTypeIndex = HeapIndex;
    allocateInfo.allocationSize = allocate_size;
    VkResult allocation_status = vkAllocateMemory(mem_context->m_device, &allocateInfo, VK_NULL_HANDLE, &alloc->m_memory);
    if (allocation_status == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        alloc->m_created_flag = alloc->m_size = alloc->m_free_offset = 0;
        printf("Vulkan MEMORY ALLOCATION Failed with error code VK_ERROR_OUT_OF_HOST_MEMORY\n");
        return 0;
    }
    else if (allocation_status == VK_ERROR_OUT_OF_DEVICE_MEMORY)
    {
        alloc->m_created_flag = alloc->m_size = alloc->m_free_offset = 0;
        printf("Vulkan MEMORY ALLOCATION Failed with error code VK_ERROR_OUT_OF_DEVICE_MEMORY\n");
        return 0;
    }
    else if (allocation_status != VK_SUCCESS)
    {
        alloc->m_created_flag = alloc->m_size = alloc->m_free_offset = 0;
        printf("Vulkan MEMORY ALLOCATION Failed with error code '%d'\n", allocation_status);
        return 0;
    }
    alloc->m_allocation_properties = allocation_memory_properties;
#if defined(_DEBUG)
    printf("Vulkan Memory Allocator: new '%d' megabytes memory block allocated, heap properties: ", VULKAN_MEMORY_BYTE_TO_MB(allocate_size));
    char heap_properties[512];
    heap_properties[0] = 0;
    if (allocation_memory_properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        strcat(heap_properties, "DEVICE_LOCAL_BIT ");
    if (allocation_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        strcat(heap_properties, "HOST_VISIBLE_BIT ");
    if (allocation_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        strcat(heap_properties, "HOST_COHERENT_BIT ");
    if (allocation_memory_properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        strcat(heap_properties, "HOST_CACHED_BIT ");
    if (allocation_memory_properties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
        strcat(heap_properties, "LAZILY_ALLOCATED_BIT ");
    if (allocation_memory_properties & VK_MEMORY_PROPERTY_PROTECTED_BIT)
        strcat(heap_properties, "PROTECTED_BIT ");
    if (allocation_memory_properties & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
        strcat(heap_properties, "DEVICE_COHERENT_BIT_AMD ");
    if (allocation_memory_properties & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
        strcat(heap_properties, "DEVICE_UNCACHED_BIT_AMD ");
    printf("%s\n", heap_properties);
#endif
    return 1;
}

void vulkan_memory_free_texture2d(VulkanMemoryContext mem_context, VkImage image, uint32_t allocation_index)
{
    assert(allocation_index < VULKAN_MEMORY_MAX_ALLOCATIONS && mem_context && image);
    struct VulkanMemoryAllocation *alloc = &mem_context->m_allocations[allocation_index];
    assert(alloc->m_created_flag);
    for (size_t i = 0; i < cvector_size(alloc->m_allocated_buffers); i++)
    {
        struct VulkanMemorySubAllocation *image_alloc = &alloc->m_allocated_buffers[i];
        if (image == image_alloc->m_image)
        {
            vkDestroyImage(mem_context->m_device, image, mem_context->m_allocation_callbacks);
            image_alloc->m_buffer_status = 0;
            if (i != cvector_size(alloc->m_allocated_buffers) - 1) // its the the last buffer, then it is not fragmented
                alloc->m_fragment_flag = 1;
            else
            {
                alloc->m_free_offset -= image_alloc->m_size;
            }
            break;
        }
    }
    mem_context->m_defragment_threshold++;
    if (mem_context->m_defragment_threshold >= VULKAN_MEMORY_DEFRAGMENT_THRESHOLD)
    {
        vulkan_memory_defragment_memory(mem_context);
    }
}

void vulkan_memory_defragment_memory(VulkanMemoryContext mem_context)
{
    mem_context->m_defragment_threshold = 0;
    printf("[Vulkan Allocator] Defragmenting vulkan memory heaps...");
    printf("Waiting idle...\n");
    vkDeviceWaitIdle(mem_context->m_device);
    // DEFRAG
    for (int allocation_index = 0; allocation_index < VULKAN_MEMORY_MAX_ALLOCATIONS; allocation_index++)
    {
        struct VulkanMemoryAllocation *pAlloc = &mem_context->m_allocations[allocation_index];
        if (pAlloc->m_created_flag && pAlloc->m_fragment_flag)
        {
            // TODO: implement me
        }
    }
    // DEFRAG
    printf("[Vulkan Allocator] Defragmenting complete.\n");
}
