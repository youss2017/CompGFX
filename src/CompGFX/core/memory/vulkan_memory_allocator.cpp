#include "vulkan_memory_allocator.hpp"
#include <vma/vk_mem_alloc.h>
#include "../vulkinc.hpp"
#include <functional>
#include <algorithm>
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <Utility/CppUtility.hpp>

namespace VkAlloc
{
	CONTEXT CreateContext(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, uint32_t HeapBlockAllocationSize, bool UsingBufferReference)
	{
		CONTEXT context = new _CONTEXT();

		context->m_device = device;
		context->m_physical_device = physicalDevice;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		context->m_HeapBlockAllocationSize = std::max(HeapBlockAllocationSize, /* Minimum blocks of 5 megabytes */ 5u * (1024u * 1024u));
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		VmaAllocatorCreateInfo createInfo{};
		createInfo.physicalDevice = physicalDevice;
		createInfo.device = device;
		createInfo.preferredLargeHeapBlockSize = 0;
		createInfo.pAllocationCallbacks = nullptr;
		createInfo.pDeviceMemoryCallbacks = nullptr;
		createInfo.pHeapSizeLimit = nullptr;
		createInfo.pVulkanFunctions = &vulkanFunctions;
		createInfo.instance = instance;
		createInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		if (UsingBufferReference)
			createInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		VkResult ret = vmaCreateAllocator(&createInfo, &context->m_allocator);

		VkPhysicalDeviceMemoryProperties memory_description;
		vkGetPhysicalDeviceMemoryProperties(context->m_physical_device, &memory_description);
		using namespace std;
		stringstream info;
		for (uint32_t i = 0; i < memory_description.memoryHeapCount; i++)
		{
			if (memory_description.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				info << "DEVICE_LOCAL_HEAP --- " << memory_description.memoryHeaps[i].size << ", " << VK_ALLOC_B_TO_MB(memory_description.memoryHeaps[i].size) << " mb" << endl;
			}
		}

		return context;
	}
	void DestroyContext(CONTEXT context)
	{
		vmaDestroyAllocator(context->m_allocator);
		delete context;
	}

	VkResult CreateBuffers(CONTEXT context, uint32_t count, BUFFER_DESCRIPTION* pDescs, BUFFER* pOutBuffers)
	{
		if (count == 0 || count == UINT32_MAX)
			return VK_SUCCESS;
		for (uint32_t i = 0; i < count; i++) {
			BUFFER buffer = new _BUFFER();
			BUFFER_DESCRIPTION desc = pDescs[i];
			buffer->m_description = desc;
			buffer->m_properties = desc.m_properties;
			VkBufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.size = desc.m_size;
			createInfo.usage = desc.m_usage;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
			createInfo.flags = 0;
			VmaAllocationCreateInfo vmaCreateInfo{};
			vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			// TODO: Look into VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
			switch (desc.m_properties) {
			case DEVICE_MEMORY_PROPERTY::CPU_ONLY:
			case DEVICE_MEMORY_PROPERTY::CPU_TO_GPU:
				vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
				vmaCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				vmaCreateInfo.requiredFlags = desc.mRequireCoherent ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
				buffer->m_suballocation.m_host_visible = true;
				break;
			case DEVICE_MEMORY_PROPERTY::GPU_ONLY:
				assert(!desc.mRequireCoherent);
				vmaCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				vmaCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				buffer->m_suballocation.m_host_visible = false;
				break;
			default:
				assert(0);
			}
			vmaCreateInfo.memoryTypeBits = UINT32_MAX;
			vmaCreateInfo.pool = nullptr;
			vmaCreateInfo.pUserData = nullptr;
			vmaCreateInfo.priority = desc.m_priority;
			VkResult result = vmaCreateBuffer(context->m_allocator, &createInfo, &vmaCreateInfo, &buffer->m_buffer, &buffer->m_suballocation.m_allocation, &buffer->m_suballocation.m_allocation_info);
			if (result != VK_SUCCESS) {
				LOG(ERR, "VMA Failed.");
				ut::DebugBreak();
				delete buffer;
				return result;
			}
			buffer->m_suballocation.m_coherent = true;
			pOutBuffers[i] = buffer;
		}
		return VK_SUCCESS;
	}

	VkResult CreateImages(CONTEXT context, uint32_t count, IMAGE_DESCRIPITION* pDescs, IMAGE* pOutImages)
	{
		if (count == 0 || count == UINT32_MAX)
			return VK_SUCCESS;
		for (uint32_t i = 0; i < count; i++)
		{
			IMAGE_DESCRIPITION desc = pDescs[i];
			IMAGE image = new _IMAGE();
			VkImageCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = desc.m_flags;
			createInfo.imageType = desc.m_imageType;
			createInfo.format = desc.m_format;
			createInfo.extent = desc.m_extent;
			createInfo.mipLevels = desc.m_mipLevels;
			createInfo.arrayLayers = desc.m_arrayLayers;
			createInfo.samples = desc.m_samples;
			createInfo.tiling = desc.m_tiling;
			createInfo.usage = desc.m_usage;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
			createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VmaAllocationCreateInfo vmaCreateInfo{};
			vmaCreateInfo.usage = desc.m_properties == DEVICE_MEMORY_PROPERTY::CPU_ONLY ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			// TODO: Look into VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
			switch (desc.m_properties) {
			case DEVICE_MEMORY_PROPERTY::CPU_ONLY:
				vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
				vmaCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				vmaCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
				image->m_suballocation.m_host_visible = true;
				break;
			case DEVICE_MEMORY_PROPERTY::CPU_TO_GPU:
				vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
				vmaCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				vmaCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
				image->m_suballocation.m_host_visible = true;
				break;
			case DEVICE_MEMORY_PROPERTY::GPU_ONLY:
				vmaCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				vmaCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				image->m_suballocation.m_host_visible = false;
				break;
			default:
				assert(0);
			}
			if (desc.m_lazyAllocate)
				vmaCreateInfo.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
			vmaCreateInfo.memoryTypeBits = UINT32_MAX;
			vmaCreateInfo.pool = nullptr;
			vmaCreateInfo.pUserData = nullptr;
			vmaCreateInfo.priority = desc.m_priority;
			VkResult result;
			result = vmaCreateImage(context->m_allocator, &createInfo, &vmaCreateInfo, &image->m_image, &image->m_suballocation.m_allocation, &image->m_suballocation.m_allocation_info);
			if (result != VK_SUCCESS)
			{
				delete image;
				return result;
			}
			pOutImages[i] = image;
		}
		return VK_SUCCESS;
	}

	void MapBuffer(CONTEXT context, BUFFER buffer)
	{
		assert(buffer->m_suballocation.m_host_visible && "Buffer must be host visible, CPU_ONLY OR CPU_TO_GPU!");
		vmaMapMemory(context->m_allocator, buffer->m_suballocation.m_allocation, &buffer->m_suballocation.m_allocation_info.pMappedData);
	}
	void FlushBuffer(CONTEXT context, BUFFER buffer, size_t offset, size_t size)
	{
		assert(buffer->m_suballocation.m_host_visible && "Buffer must be host visible, CPU_ONLY OR CPU_TO_GPU!");
		vmaFlushAllocation(context->m_allocator, buffer->m_suballocation.m_allocation, offset, size);
	}
	void InvalidateBuffer(CONTEXT context, BUFFER buffer, size_t offset, size_t size)
	{
		assert(buffer->m_suballocation.m_host_visible && "Buffer must be host visible, CPU_ONLY OR CPU_TO_GPU!");
		if (buffer->m_suballocation.m_coherent)
			return;
		vmaInvalidateAllocation(context->m_allocator, buffer->m_suballocation.m_allocation, offset, size);
	}
	void UnmapBuffer(CONTEXT context, BUFFER buffer)
	{
		assert(buffer->m_suballocation.m_host_visible && "Buffer must be host visible, CPU_ONLY OR CPU_TO_GPU!");
		vmaUnmapMemory(context->m_allocator, buffer->m_suballocation.m_allocation);
	}
	void DestroyBuffers(const CONTEXT context, uint32_t count, const BUFFER* pBuffers)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			BUFFER buffer = pBuffers[i];
			vmaDestroyBuffer(context->m_allocator, buffer->m_buffer, buffer->m_suballocation.m_allocation);
			delete buffer;
		}
	}
	void DestroyImages(const CONTEXT context, uint32_t count, const IMAGE* pImages)
	{
		if (!count)
			return;
		for (uint32_t i = 0; i < count; i++)
		{
			IMAGE image = pImages[i];
			vmaDestroyImage(context->m_allocator, image->m_image, image->m_suballocation.m_allocation);
			delete image;
		}
	}
	void Defragment(CONTEXT context)
	{
		//vmaBeginDefragmentation()
	}

}