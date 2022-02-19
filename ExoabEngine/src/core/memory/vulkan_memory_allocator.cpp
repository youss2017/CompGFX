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
		std::vector<DEVICE_HEAP>& heaps = context->m_device_heap;
		for (auto& heap : heaps)
		{
			for (auto& _suballoc : heap->m_suballocations)
			{
				logwarning("You should be freeing all buffers and images before destroying VkAlloc::CONTEXT");
				DEVICE_HEAP_SUBALLOCATION& suballoc = _suballoc.second;
				if (suballoc.m_buffer) {
					vkDestroyBuffer(context->m_device, ((BUFFER)suballoc.m_buffer)->m_buffer, nullptr);
				}
				else {
					vkDestroyImage(context->m_device, ((IMAGE)suballoc.m_image)->m_image, nullptr);
				}
			}
			delete heap;
			vkFreeMemory(context->m_device, heap->m_memory, nullptr);
			delete heap;
		}
		delete context;
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
		using namespace std;
		stringstream info;

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
			return nullptr;
		}
		info << "[Memory Allocator] Allocated " << VK_ALLOC_B_TO_MB(size) << " mb with memory properties " << ((ChoosenPreferred) ? preferred_string : required_string) << "\n";
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

	bool SearchHeapsAndSuballocate(CONTEXT context, std::vector<DEVICE_HEAP>& heaps, std::vector<BUFFER_DESCRIPTION> descs, std::vector<IMAGE_DESCRIPITION> idescs, std::vector<BUFFER>& buffers, std::vector<IMAGE>& images, bool IsBuffers)
	{
		if (descs.size() == 0)
			return true;

		// They all have the same property because they were seperated.
		DEVICE_MEMORY_PROPERTY properties = descs[0].m_properties;

		// Get the size needed for each buffer and its memory type (used for next heap allocation)
		VkDeviceSize requiredSize = 0;
		if (IsBuffers) {
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
				buffer->m_description = desc;
				buffers.push_back(buffer);
			}
		} else {
			for (auto& desc : idescs) {
				VkImageCreateInfo createInfo;
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
				IMAGE image = new _IMAGE();
				image->m_properties = properties;
				image->m_width = desc.m_extent.width;
				image->m_height = desc.m_extent.height;
				image->m_sampleCount = desc.m_samples;
				vkCreateImage(context->m_device, &createInfo, nullptr, &image->m_image);
				vkGetImageMemoryRequirements(context->m_device, image->m_image, &image->m_memrequirements);
				image->m_suballocation.m_size = image->m_memrequirements.size;
				requiredSize += image->m_memrequirements.size;
				image->m_description = desc;
				images.push_back(image);
			}
		}

		int i = 0;
		if (IsBuffers) {
			for (auto& d : descs)
			{
				VkMemoryRequirements memRequirement;
				vkGetBufferMemoryRequirements(context->m_device, buffers[i]->m_buffer, &memRequirement);
				buffers[i]->m_memrequirements = memRequirement;
				buffers[i]->m_suballocation.m_size = AlignMemory(d.m_size, memRequirement.alignment);
				requiredSize += AlignMemory(d.m_size, memRequirement.alignment);
				i++;
			}
		}

		/*
			Note: Returning true means there is more space in the current heap; keep allocating.
		*/

		auto BufferSuballocationProcess = [context](DEVICE_HEAP& heap, BUFFER buffer, int& next_buffer_image_index) throw() -> bool {
			for (int block_index = 0; block_index < heap->m_free_blocks.size(); block_index++)
			{
				DEVICE_HEAP_FREE_BLOCK& block = heap->m_free_blocks[block_index];
				uint32_t block_size = block.m_size - (AlignMemory(block.m_offset, buffer->m_memrequirements.alignment) - block.m_offset);
				if (block_size >= buffer->m_suballocation.m_size)
				{
					if (AlignMemory(block.m_offset, buffer->m_memrequirements.alignment) - block.m_offset > 0)
					{
						heap->m_free_blocks.push_back({ block.m_offset, AlignMemory(block.m_offset, buffer->m_memrequirements.alignment) - block.m_offset });
					}
					buffer->m_heap_free_block.m_offset = AlignMemory(block.m_offset, buffer->m_memrequirements.alignment);
					buffer->m_heap_free_block.m_size = buffer->m_suballocation.m_size;
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
					next_buffer_image_index++;
					return true;
				}
			}
			return false;
		};

		auto ImageSuballocationProcess = [context](DEVICE_HEAP& heap, IMAGE image, int& next_buffer_image_index) throw() -> bool {
			for (int block_index = 0; block_index < heap->m_free_blocks.size(); block_index++)
			{
				DEVICE_HEAP_FREE_BLOCK& block = heap->m_free_blocks[block_index];
				if (block.m_size >= image->m_suballocation.m_size)
				{
					image->m_heap_free_block.m_offset = block.m_offset;
					image->m_heap_free_block.m_size = image->m_memrequirements.size;
					image->m_suballocation.m_offset = block.m_offset;
					image->m_suballocation.m_coherent = heap->m_coherent;
					image->m_suballocation.m_host_visible = heap->m_host_visible;
					image->m_suballocation.m_memory = heap->m_memory;
					image->m_suballocation.m_heap_index = heap->m_heap_index;
					image->m_suballocation.m_suballocation_index = heap->m_suballocations.size();
					image->m_suballocation.m_image = image;
					vkBindImageMemory(context->m_device, image->m_image, heap->m_memory, block.m_offset);
					block.m_offset += image->m_suballocation.m_size;
					block.m_size -= image->m_suballocation.m_size;
					heap->m_suballocations.insert(std::make_pair(image->m_suballocation.m_suballocation_index, image->m_suballocation));
					next_buffer_image_index++;
					return true;
				}
			}
			return false;
		};

		i = 0;
		// counter to determine the next buffer to bind memory to
		int next_buffer_image_index = 0;
		// Searches through heap to find a suitable heap (with required memory properties)
		// Finds if it has enough available memory 
		for (auto& heap : heaps)
		{
			if (properties != heap->m_properties)
				continue;
			suballocate:
			if (next_buffer_image_index >= buffers.size())
				break;
			BUFFER buffer = nullptr;
			IMAGE image = nullptr;

			if (IsBuffers) {
				buffer = buffers[next_buffer_image_index];
				if (BufferSuballocationProcess(heap, buffer, next_buffer_image_index))
					goto suballocate;
			}
			else {
				image = images[next_buffer_image_index];
				if (ImageSuballocationProcess(heap, image, next_buffer_image_index))
					goto suballocate;
			}
			i++;
		}

		if (next_buffer_image_index < buffers.size())
		{
			// Allocate a new heap
			DEVICE_HEAP heap = CreateHeap(context, buffers[0]->m_memrequirements, properties, std::max(context->m_HeapBlockAllocationSize, (uint32_t)requiredSize));
			if (!heap) {
				// TODO: Destroy objects that were created
				return false;
			}
			heaps.push_back(heap);
			suballocate2:
			if (next_buffer_image_index >= buffers.size())
				return true;
			if (IsBuffers) {
				BUFFER buffer = buffers[next_buffer_image_index];
				if (BufferSuballocationProcess(heap, buffer, next_buffer_image_index))
					goto suballocate2;
			}
			else {
				IMAGE image = images[next_buffer_image_index];
				if (ImageSuballocationProcess(heap, image, next_buffer_image_index))
					goto suballocate2;
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
		std::vector<IMAGE> empty;
		bool result1 = SearchHeapsAndSuballocate(context, heaps, CpuOnly, {}, buffers, empty, true);
		bool result2 = SearchHeapsAndSuballocate(context, heaps, CpuToGpu, {}, buffers, empty, true);
		bool result3 = SearchHeapsAndSuballocate(context, heaps, GpuOnly, {},buffers, empty, true);
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
		std::vector<BUFFER> empty;
		bool result1 = SearchHeapsAndSuballocate(context, heaps, {}, CpuOnly, empty, images, true);
		bool result2 = SearchHeapsAndSuballocate(context, heaps, {}, CpuToGpu, empty, images, true);
		bool result3 = SearchHeapsAndSuballocate(context, heaps, {}, GpuOnly, empty, images, true);
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
							if(image->m_description.m_extent.depth == pDescs[j].m_extent.depth)
								if (image->m_sampleCount == pDescs[j].m_samples)
									break;
			}
			pOutImages[i] = images[j];
		}
		return VK_TRUE;
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
		assert(!buffer->m_suballocation.m_mapped_pointer && "Buffer must have been flushed before its unmapped.");
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
			std::vector<DEVICE_HEAP>& heaps = context->m_device_heap;
			DEVICE_HEAP& heap = heaps[buffer->m_suballocation.m_heap_index];
			heap->m_free_blocks.push_back(buffer->m_heap_free_block);
			// If a new block is added that means there is a discontunity in memory
			// therefore there is gabs between free memory blocks
			// this helps determine whether the heap needs to be defragmented.
			heap->m_fragmentation_score++;
			if (buffer->m_suballocation.m_host_visible)
				UnmapBuffer(context, buffer);
			heap->m_suballocations.erase(buffer->m_suballocation.m_suballocation_index);
			vkDestroyBuffer(context->m_device, buffer->m_buffer, nullptr);
			delete buffer;
		}
	}
	void DestroyImages(CONTEXT context, uint32_t count, IMAGE* pImages)
	{
		if (!count)
			return;
		for (int i = 0; i < count; i++)
		{
			IMAGE image = pImages[i];
			vkDestroyImage(context->m_device, image->m_image, nullptr);
			DEVICE_HEAP& heap = context->m_device_heap[image->m_suballocation.m_heap_index];
			heap->m_free_blocks.push_back(image->m_heap_free_block);
			heap->m_fragmentation_score++;
			heap->m_suballocations.erase(image->m_suballocation.m_suballocation_index);
			delete image;
		}
	}
	void Defragment(CONTEXT context)
	{
		std::vector<DEVICE_HEAP>& heaps = context->m_device_heap;
		std::vector<DEVICE_HEAP>::iterator iter = heaps.begin();
		for (DEVICE_HEAP& heap : heaps)
		{
			if (heap->m_suballocations.size() == 0)
			{
				vkFreeMemory(context->m_device, heap->m_memory, nullptr);
				delete heap;
				heaps.erase(iter, ++iter);
				continue;
			}
			// join the free blocks
			std::sort(heap->m_free_blocks.begin(), heap->m_free_blocks.end(), [](const DEVICE_HEAP_FREE_BLOCK& a, const DEVICE_HEAP_FREE_BLOCK& b) -> bool {return a.m_offset < b.m_offset; });
			std::vector<DEVICE_HEAP_FREE_BLOCK> joined_blocks;
			std::vector<DEVICE_HEAP_FREE_BLOCK>& blocks = heap->m_free_blocks;
			for (int i = 0; i < blocks.size() - 1; i++) {
				if (blocks[i].m_offset + blocks[i].m_size == blocks[i + 1].m_offset) {
					joined_blocks.push_back({ blocks[i].m_offset, blocks[i].m_size + blocks[i + 1].m_size });
				}
				else {
					joined_blocks.push_back(blocks[i]);
				}
			}
			heap->m_fragmentation_score = 0;
			heap->m_free_blocks = joined_blocks;
			iter++;
		}
	}

}