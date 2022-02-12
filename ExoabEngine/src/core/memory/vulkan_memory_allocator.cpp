#include "vulkan_memory_allocator.hpp"
#include <backend/VulkanLoader.h>
#include <Logger.hpp>
#include <functional>
#include <algorithm>
#include <cassert>
#include <string>
#include <sstream>

namespace VkAlloc
{
	CONTEXT CreateContext(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t HeapBlockAllocationSize)
	{
		CONTEXT context = new _CONTEXT();

		context->m_device = device;
		context->m_physical_device = physicalDevice;
		
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		context->m_max_allocations = properties.limits.maxMemoryAllocationCount;
		context->m_minMemoryMapAlignment = properties.limits.minMemoryMapAlignment;
		context->m_minTexelBufferOffsetAlignment = properties.limits.minTexelBufferOffsetAlignment;
		context->m_minUniformBufferOffsetAlignment = properties.limits.minUniformBufferOffsetAlignment;
		context->m_minStorageBufferOffsetAlignment = properties.limits.minStorageBufferOffsetAlignment;
		context->m_HeapBlockAllocationSize = std::max(HeapBlockAllocationSize, /* Minimum blocks of 5 megabytes */ 5u * (1024u * 1024u));
		context->m_next_heap_index = 0;

		return context;
	}
	void DestroyContext(CONTEXT context)
	{
	}

	static inline uint32_t AlignMemory(uint32_t buffer_size, uint32_t alignment)
	{
		if (buffer_size % alignment != 0)
			buffer_size = (buffer_size - (buffer_size % alignment)) + alignment;
		return buffer_size;
	}

	template<class T>
	static inline void Seperate(uint32_t count, T* elements, std::vector<T>& output, std::function<bool(void* e)> func)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			if (func(&elements[i]))
				output.push_back(elements[i]);
		}
	}

	static uint32_t vulkan_memory_find_heap_index(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties, uint32_t* pOutHeapIndex)
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

	static DEVICE_HEAP CreateHeap(CONTEXT context, VkMemoryRequirements memRequirement, DEVICE_MEMORY_PROPERTY properties, uint32_t size)
	{
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

		VkMemoryPropertyFlags memory_flags_preferred;
		VkMemoryPropertyFlags memory_flags_required;

		string preferred_string;
		string required_string;

		switch (properties)
		{
			case DEVICE_MEMORY_PROPERTY::CPU_ONLY:
				memory_flags_preferred = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				memory_flags_required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
				preferred_string = "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT "s + "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT";
				required_string = "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ";
				break;
			case DEVICE_MEMORY_PROPERTY::CPU_TO_GPU:
				memory_flags_preferred = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				memory_flags_required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				preferred_string = "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT "s + "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT "s + "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT";
				required_string = "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT "s + "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT";
				break;
			case DEVICE_MEMORY_PROPERTY::GPU_ONLY:
				memory_flags_preferred = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				memory_flags_required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				preferred_string = "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT";
				required_string = "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT";
				break;
			default:
				log_warning("Invalid DEVICE_MEMORY_PROPERTY flag", true);
				memory_flags_preferred = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				memory_flags_required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}

		uint32_t memoryTypeIndex = 0;
		bool ChoosenPreferred = true;
		if (!vulkan_memory_find_heap_index(memRequirement.memoryTypeBits, context->m_physical_device, memory_flags_preferred, &memoryTypeIndex))
		{
			ChoosenPreferred = false;
			if (!vulkan_memory_find_heap_index(memRequirement.memoryTypeBits, context->m_physical_device, memory_flags_required, &memoryTypeIndex))
			{
				logerror("[Vulkan] Could not find memory type heap index. Device Heap Creation failed!");
				throw std::runtime_error("Device Heap Creation failed.");
			}
		}

		VkMemoryAllocateInfo allocInfo;
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = size;
		allocInfo.memoryTypeIndex = memoryTypeIndex;
		VkDeviceMemory memory;
		VkResult result = vkAllocateMemory(context->m_device, &allocInfo, nullptr, &memory);
		if (result != VK_SUCCESS)
		{
			info << "FAILED TO ALLOCATE MEMORY. " << __FILE__ << ":" << __LINE__ << endl;
			lograws(info.str());
			return nullptr;
		}
		lograws(info.str());
		DEVICE_HEAP heap = new _DEVICE_HEAP();
		heap->m_free_blocks.push_back({ 0, size });
		heap->m_memory = memory;
		heap->m_size = size;
		heap->m_properties = properties;
		heap->m_coherent = ChoosenPreferred ? memory_flags_preferred & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : memory_flags_required & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		heap->m_host_visible = ChoosenPreferred ? memory_flags_preferred & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : memory_flags_required & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		heap->m_heap_index = context->m_next_heap_index++;
		return heap;
	}

	bool SearchHeapsAndSuballocate(CONTEXT context, std::vector<DEVICE_HEAP>& heaps, std::vector<BUFFER_DESCRIPTION>& descs, std::vector<BUFFER>& buffers)
	{
		if (descs.size() == 0)
			return true;

		// They all have the same property because they were seperated.
		DEVICE_MEMORY_PROPERTY properties = descs[0].m_properties;

		// Create BUFFERs
		for (auto& desc : descs)
		{
			VkBufferCreateInfo createInfo;
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.size = desc.m_size;
			createInfo.usage = desc.m_usage;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
			BUFFER buffer = new _BUFFER();
			buffer->m_properties = properties;
			vkCreateBuffer(context->m_device, &createInfo, nullptr, &buffer->m_buffer);
			buffers.push_back(buffer);
		}

		VkDeviceSize requiredSize = 0;
		// Get the size needed for each buffer and its memory type (used for next heap allocation)
		int i = 0;
		for (auto& d : descs)
		{
			VkMemoryRequirements memRequirement;
			vkGetBufferMemoryRequirements(context->m_device, buffers[i]->m_buffer, &memRequirement);
			buffers[i]->m_memrequirements = memRequirement;
			buffers[i]->m_suballocation.m_size = AlignMemory(d.m_size, memRequirement.alignment);
			requiredSize += AlignMemory(d.m_size, memRequirement.alignment);
			i++;
		}
		i = 0;
		// counter to determine the next buffer to bind memory to
		int next_buffer_index = 0;
		// Searches through heap to find a suitable heap (with required memory properties)
		// Finds if it has enough available memory 
		for (auto& heap : heaps)
		{
			if (properties != heap->m_properties)
				continue;
			suballocate:
			if (next_buffer_index >= buffers.size())
				break;
			BUFFER buffer = buffers[next_buffer_index];
			
			for (int block_index = 0; block_index < heap->m_free_blocks.size(); block_index++)
			{
				DEVICE_HEAP_FREE_BLOCK& block = heap->m_free_blocks[block_index];
				uint32_t block_size = block.m_size - (AlignMemory(block.m_offset, buffer->m_memrequirements.alignment) - block.m_offset);
				if (block_size >= buffer->m_suballocation.m_size)
				{
					buffer->m_suballocation.m_offset = AlignMemory(block.m_offset, buffer->m_memrequirements.alignment);
					buffer->m_suballocation.m_coherent = heap->m_coherent;
					buffer->m_suballocation.m_host_visible = heap->m_host_visible;
					buffer->m_suballocation.m_memory = heap->m_memory;
					buffer->m_suballocation.m_heap_index = heap->m_heap_index;
					buffer->m_suballocation.m_suballocation_index = heap->m_suballocations.size();
					buffer->m_suballocation.m_buffer = buffer;
					vkBindBufferMemory(context->m_device, buffer->m_buffer, heap->m_memory, AlignMemory(block.m_offset, buffer->m_memrequirements.alignment));
					block.m_offset += buffer->m_suballocation.m_size;
					heap->m_suballocations.insert(std::make_pair(buffer->m_suballocation.m_suballocation_index, buffer->m_suballocation));
					next_buffer_index++;
					goto suballocate;
				}
			}
			i++;
		}

		if (next_buffer_index < buffers.size())
		{
			// Allocate a new heap
			DEVICE_HEAP heap = CreateHeap(context, buffers[0]->m_memrequirements, properties, std::max(context->m_HeapBlockAllocationSize, (uint32_t)requiredSize));
			if (!heap) {
				// TODO: Destroy objects that were created
				return false;
			}
			heaps.push_back(heap);
			suballocate2:
			if (next_buffer_index >= buffers.size())
				return true;
			BUFFER buffer = buffers[next_buffer_index];

			for (int block_index = 0; block_index < heap->m_free_blocks.size(); block_index++)
			{
				DEVICE_HEAP_FREE_BLOCK& block = heap->m_free_blocks[block_index];
				uint32_t block_size = block.m_size - (AlignMemory(block.m_offset, buffer->m_memrequirements.alignment) - block.m_offset);
				if (block_size >= buffer->m_suballocation.m_size)
				{
					buffer->m_suballocation.m_offset = AlignMemory(block.m_offset, buffer->m_memrequirements.alignment);
					buffer->m_suballocation.m_coherent = heap->m_coherent;
					buffer->m_suballocation.m_host_visible = heap->m_host_visible;
					buffer->m_suballocation.m_memory = heap->m_memory;
					buffer->m_suballocation.m_heap_index = heap->m_heap_index;
					buffer->m_suballocation.m_suballocation_index = heap->m_suballocations.size();
					buffer->m_suballocation.m_buffer = buffer;
					vkBindBufferMemory(context->m_device, buffer->m_buffer, heap->m_memory, AlignMemory(block.m_offset, buffer->m_memrequirements.alignment));
					block.m_offset += buffer->m_suballocation.m_size;
					heap->m_suballocations.insert(std::make_pair(buffer->m_suballocation.m_suballocation_index, buffer->m_suballocation));
					next_buffer_index++;
					goto suballocate2;
				}
			}
		}

		return true;
	}

	VkBool32 CreateBuffers(CONTEXT context, uint32_t count, BUFFER_DESCRIPTION* pDescs, BUFFER* pOutBuffers)
	{
		if (count == 0 || count == UINT32_MAX)
			return VK_TRUE;
		// 1) Sort Buffers by memory requirements
		std::vector<BUFFER_DESCRIPTION> CpuOnly;
		std::vector<BUFFER_DESCRIPTION> CpuToGpu;
		std::vector<BUFFER_DESCRIPTION> GpuOnly;
		Seperate(count, pDescs, CpuOnly, [](void* e) throw() -> bool { if (((BUFFER_DESCRIPTION*)e)->m_properties == DEVICE_MEMORY_PROPERTY::CPU_ONLY) return true; else return false; });
		Seperate(count, pDescs, CpuToGpu, [](void* e) throw() -> bool { if (((BUFFER_DESCRIPTION*)e)->m_properties == DEVICE_MEMORY_PROPERTY::CPU_TO_GPU) return true; else return false; });
		Seperate(count, pDescs, GpuOnly, [](void* e) throw() -> bool { if (((BUFFER_DESCRIPTION*)e)->m_properties == DEVICE_MEMORY_PROPERTY::GPU_ONLY) return true; else return false; });
		// 2) Sort buffers by size
		//		(For GpuOnly) sort from biggest to smallest because smaller buffers are more likely to be deleted faster
		//		(For the other) sort from smallest to biggest because bigger buffers are more likely used for staging
		std::sort(CpuOnly.begin(), CpuOnly.end(), [](const BUFFER_DESCRIPTION& a, const BUFFER_DESCRIPTION& b) -> bool { if (a.m_size < b.m_size) return true; else return false; });
		std::sort(CpuToGpu.begin(), CpuToGpu.end(), [](const BUFFER_DESCRIPTION& a, const BUFFER_DESCRIPTION& b) -> bool { if (a.m_size < b.m_size) return true; else return false; });
		std::sort(GpuOnly.begin(), GpuOnly.end(), [](const BUFFER_DESCRIPTION& a, const BUFFER_DESCRIPTION& b) -> bool { if (a.m_size > b.m_size) return true; else return false; });
		// 3) Check If heaps already exist
		std::vector<DEVICE_HEAP>& heaps = context->m_device_heap;
		std::vector<BUFFER> buffers;
		bool result1 = SearchHeapsAndSuballocate(context, heaps, CpuOnly, buffers);
		bool result2 = SearchHeapsAndSuballocate(context, heaps, CpuToGpu, buffers);
		bool result3 = SearchHeapsAndSuballocate(context, heaps, GpuOnly, buffers);
		if (!result1 || !result2 || !result3)
		{
			// TODO: Destroy objects that were created.
			return VK_FALSE;
		}
		// Write BUFFERs in order (as the user expects)
		for (uint32_t i = 0; i < count; i++)
		{
			int j = 0;
			for (; j < buffers.size(); j++)
			{
				BUFFER buffer = buffers[j];
				if (buffer->m_properties == pDescs[j].m_properties)
					if (buffer->m_suballocation.m_size == AlignMemory(pDescs[j].m_size, buffer->m_memrequirements.alignment))
						break;
			}
			pOutBuffers[i] = buffers[j];
		}
		return VK_TRUE;
	}
	
	static bool SearchHeapsAndSuballocateIMAGE(CONTEXT context, std::vector<DEVICE_HEAP>& heaps, std::vector<IMAGE_DESCRIPITION>& descs, std::vector<IMAGE>& images)
	{
		if (descs.size() == 0)
			return true;

		// They all have the same property because they were seperated.
		DEVICE_MEMORY_PROPERTY properties = descs[0].m_properties;
		VkDeviceSize requiredSize = 0;
		for (int i = 0; i < descs.size(); i++)
		{
			VkImageCreateInfo createInfo;
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = descs[i].m_flags;
			createInfo.imageType = descs[i].m_imageType;
			createInfo.format = descs[i].m_format;
			createInfo.extent = descs[i].m_extent;
			createInfo.mipLevels = descs[i].m_mipLevels;
			createInfo.arrayLayers = descs[i].m_arrayLayers;
			createInfo.samples = descs[i].m_samples;
			createInfo.tiling = descs[i].m_tiling;
			createInfo.usage = descs[i].m_usage;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
			createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			IMAGE image = new _IMAGE();
			image->m_properties = properties;
			image->m_width = descs[i].m_extent.width;
			image->m_height = descs[i].m_extent.height;
			image->m_sampleCount = descs[i].m_samples;
			vkCreateImage(context->m_device, &createInfo, nullptr, &image->m_image);
			vkGetImageMemoryRequirements(context->m_device, image->m_image, &image->m_memrequirements);
			image->m_suballocation.m_size = image->m_memrequirements.size;
			requiredSize += image->m_memrequirements.size;
			images.push_back(image);
		}

		// counter to determine the next image to bind memory to
		int next_image_index = 0;
		// Searches through heap to find a suitable heap (with required memory properties)
		// Finds if it has enough available memory 
		int i = 0;
		for (auto& heap : heaps)
		{
			if (properties != heap->m_properties)
				continue;
			suballocate:
			if (next_image_index >= images.size())
				break;
			IMAGE image = images[next_image_index];
			
			for (int block_index = 0; block_index < heap->m_free_blocks.size(); block_index++)
			{
				DEVICE_HEAP_FREE_BLOCK& block = heap->m_free_blocks[block_index];
				if (block.m_size >= image->m_suballocation.m_size)
				{
					image->m_suballocation.m_offset = block.m_offset;
					image->m_suballocation.m_coherent = heap->m_coherent;
					image->m_suballocation.m_host_visible = heap->m_host_visible;
					image->m_suballocation.m_memory = heap->m_memory;
					image->m_suballocation.m_heap_index = heap->m_heap_index;
					image->m_suballocation.m_suballocation_index = heap->m_suballocations.size();
					image->m_suballocation.m_image = image;
					vkBindImageMemory(context->m_device, image->m_image, heap->m_memory, block.m_offset);
					block.m_offset += image->m_suballocation.m_size;
					heap->m_suballocations.insert(std::make_pair(image->m_suballocation.m_suballocation_index, image->m_suballocation));
					next_image_index++;
					goto suballocate;
				}
			}
			i++;
		}

		if (next_image_index < images.size())
		{
			// Allocate a new heap
			DEVICE_HEAP heap = CreateHeap(context, images[0]->m_memrequirements, properties, std::max(context->m_HeapBlockAllocationSize, (uint32_t)requiredSize));
			if (!heap) {
				// TODO: Destroy objects that were created
				return false;
			}
			heaps.push_back(heap);
			suballocate2:
			if (next_image_index >= images.size())
				return true;
			IMAGE image = images[next_image_index];

			for (int block_index = 0; block_index < heap->m_free_blocks.size(); block_index++)
			{
				DEVICE_HEAP_FREE_BLOCK& block = heap->m_free_blocks[block_index];
				if (block.m_size >= image->m_suballocation.m_size)
				{
					image->m_suballocation.m_offset = block.m_offset;
					image->m_suballocation.m_coherent = heap->m_coherent;
					image->m_suballocation.m_host_visible = heap->m_host_visible;
					image->m_suballocation.m_memory = heap->m_memory;
					image->m_suballocation.m_heap_index = heap->m_heap_index;
					image->m_suballocation.m_suballocation_index = heap->m_suballocations.size();
					image->m_suballocation.m_image = image;
					vkBindImageMemory(context->m_device, image->m_image, heap->m_memory, block.m_offset);
					block.m_offset += image->m_suballocation.m_size;
					heap->m_suballocations.insert(std::make_pair(image->m_suballocation.m_suballocation_index, image->m_suballocation));
					next_image_index++;
					goto suballocate2;
				}
			}
		}

		return true;
	}
	
	VkBool32 CreateImages(CONTEXT context, uint32_t count, IMAGE_DESCRIPITION* pDescs, IMAGE* pOutImages)
	{
		if (count == 0 || count == UINT32_MAX)
			return VK_TRUE;
		// 1) Sort Buffers by memory requirements
		std::vector<IMAGE_DESCRIPITION> CpuOnly;
		std::vector<IMAGE_DESCRIPITION> CpuToGpu;
		std::vector<IMAGE_DESCRIPITION> GpuOnly;
		Seperate(count, pDescs, CpuOnly, [](void* e) throw() -> bool { if (((IMAGE_DESCRIPITION*)e)->m_properties == DEVICE_MEMORY_PROPERTY::CPU_ONLY) return true; else return false; });
		Seperate(count, pDescs, CpuToGpu, [](void* e) throw() -> bool { if (((IMAGE_DESCRIPITION*)e)->m_properties == DEVICE_MEMORY_PROPERTY::CPU_TO_GPU) return true; else return false; });
		Seperate(count, pDescs, GpuOnly, [](void* e) throw() -> bool { if (((IMAGE_DESCRIPITION*)e)->m_properties == DEVICE_MEMORY_PROPERTY::GPU_ONLY) return true; else return false; });
		// 2) Search Buffers
		std::vector<DEVICE_HEAP>& heaps = context->m_device_heap;
		std::vector<IMAGE> images;
		bool result1 = SearchHeapsAndSuballocateIMAGE(context, heaps, CpuOnly, images);
		bool result2 = SearchHeapsAndSuballocateIMAGE(context, heaps, CpuToGpu, images);
		bool result3 = SearchHeapsAndSuballocateIMAGE(context, heaps, GpuOnly, images);
		// 3) Write IMAGEs in order (as the user expects)
		for (uint32_t i = 0; i < count; i++)
		{
			int j = 0;
			for (; j < images.size(); j++)
			{
				IMAGE image = images[j];
				if (image->m_properties == pDescs[j].m_properties)
					if (image->m_width == pDescs[j].m_extent.width)
						if (image->m_height == pDescs[j].m_extent.height)
							if (image->m_sampleCount == pDescs[j].m_samples)
								break;
			}
			pOutImages[i] = images[j];
		}
		return VK_TRUE;
	}
	void ReAllocBuffer(CONTEXT context, BUFFER buffer, size_t new_size)
	{
		// 1) Store buffer contexts into memory
		char8_t* host_memory = nullptr;
		if (buffer->m_suballocation.m_host_visible)
		{
			MapBuffer(context, buffer);
			memcpy(host_memory, buffer->m_suballocation.m_mapped_pointer, buffer->m_suballocation.m_size);
			UnmapBuffer(context, buffer);
		}
		else {
			assert(0 && "If you really want to realloc a non-host visible buffer do it yourself.");
		}
		//if()
	}
	void MapBuffer(CONTEXT context, BUFFER buffer)
	{
		assert(!buffer->m_suballocation.m_destroyed && "Buffer must not have been destroyed!");
		assert(buffer->m_suballocation.m_host_visible && "Buffer must be host visible, CPU_ONLY OR CPU_TO_GPU!");
		if (buffer->m_suballocation.m_mapped_pointer)
			return;
		DEVICE_HEAP& heap = context->m_device_heap[buffer->m_suballocation.m_heap_index];
		if (!heap->m_mapped_pointer)
			vkMapMemory(context->m_device, buffer->m_suballocation.m_memory, 0, VK_WHOLE_SIZE, 0, (void**)&heap->m_mapped_pointer);
		heap->m_map_count++;
		buffer->m_suballocation.m_mapped_pointer = heap->m_mapped_pointer + buffer->m_suballocation.m_offset;
	}
	void FlushBuffer(CONTEXT context, BUFFER buffer)
	{
		assert(!buffer->m_suballocation.m_destroyed && "Buffer must not have been destroyed!");
		assert(buffer->m_suballocation.m_host_visible && "Buffer must be host visible, CPU_ONLY OR CPU_TO_GPU!");
		if (buffer->m_suballocation.m_coherent)
			return;
		VkMappedMemoryRange range;
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext = nullptr;
		range.memory = buffer->m_suballocation.m_memory;
		range.offset = buffer->m_suballocation.m_offset;
		range.size = buffer->m_suballocation.m_size;
		vkFlushMappedMemoryRanges(context->m_device, 1, &range);
	}
	void InvalidateBuffer(CONTEXT context, BUFFER buffer)
	{
		assert(!buffer->m_suballocation.m_destroyed && "Buffer must not have been destroyed!");
		assert(buffer->m_suballocation.m_host_visible && "Buffer must be host visible, CPU_ONLY OR CPU_TO_GPU!");
		if (buffer->m_suballocation.m_coherent)
			return;
		VkMappedMemoryRange range;
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext = nullptr;
		range.memory = buffer->m_suballocation.m_memory;
		range.offset = buffer->m_suballocation.m_offset;
		range.size = buffer->m_suballocation.m_size;
		vkInvalidateMappedMemoryRanges(context->m_device, 1, &range);
	}
	void UnmapBuffer(CONTEXT context, BUFFER buffer)
	{
		assert(!buffer->m_suballocation.m_destroyed && "Buffer must not have been destroyed!");
		assert(buffer->m_suballocation.m_host_visible && "Buffer must be host visible, CPU_ONLY OR CPU_TO_GPU!");
		if (buffer->m_suballocation.m_mapped_pointer == nullptr)
			return;
		buffer->m_suballocation.m_mapped_pointer = nullptr;
		DEVICE_HEAP& heap = context->m_device_heap[buffer->m_suballocation.m_heap_index];
		if (--heap->m_map_count == 0)
		{
			// We only can unmap once all suballocations have unmapped.
			vkUnmapMemory(context->m_device, context->m_device_heap[buffer->m_suballocation.m_heap_index]->m_memory);
			heap->m_mapped_pointer = nullptr;
		}
	}
	void DestroyBuffers(CONTEXT context, uint32_t count, BUFFER* pBuffers)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			BUFFER buffer = pBuffers[i];
			DEVICE_HEAP_FREE_BLOCK new_block;
			new_block.m_offset = buffer->m_suballocation.m_offset;
			new_block.m_size = buffer->m_suballocation.m_size;
			std::vector<DEVICE_HEAP>& heaps = context->m_device_heap;
			DEVICE_HEAP& heap = heaps[buffer->m_suballocation.m_heap_index];

			// Join DEVICE_HEAP_FREE_BLOCK
			// Why join blocks, example:
			// Block 0: offset(0), size(1000)
			// Block 1: offset(1000), sizeo(63000)
			// After joining
			// Block 0: offset(0), sizeof(64000)
			// This allows a more efficient use of memory and reduces memory fragmentation.
			bool joined_blocks = false;
			for (auto& block : heap->m_free_blocks)
			{
				if (new_block.m_offset + new_block.m_size == block.m_offset)
				{
					block.m_offset = new_block.m_offset;
					block.m_size += new_block.m_size;
					joined_blocks = true;
					break;
				}
			}
			if (!joined_blocks) {
				heap->m_free_blocks.push_back(new_block);
				// If a new block is added that means there is a discontunity in memory
				// therefore there is gabs between free memory blocks
				// this helps determine whether the heap needs to be defragmented.
				heap->m_fragmentation_score++;
			}
			if (buffer->m_suballocation.m_host_visible)
				UnmapBuffer(context, buffer);
			heap->m_suballocations.erase(buffer->m_suballocation.m_suballocation_index);
			vkDestroyBuffer(context->m_device, buffer->m_buffer, nullptr);
			delete buffer;
		}
	}
	void DestroyImages(CONTEXT context, uint32_t count, IMAGE* pImages)
	{
	}
	void DefragmentMemory(CONTEXT context)
	{
	}

}