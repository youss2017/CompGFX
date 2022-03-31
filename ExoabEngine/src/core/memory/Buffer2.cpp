#include "Buffer2.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <unordered_map>

extern vk::VkContext gContext;

IBuffer2 Buffer2_Create(BufferType type, size_t size, BufferMemoryType memoryType, bool pointerUsage, bool requireCoherent, bool intermediateBuffer)
{
	using namespace VkAlloc;
	BUFFER_DESCRIPTION desc;
	desc.m_properties = memoryType == BufferMemoryType::GPU_ONLY ? DEVICE_MEMORY_PROPERTY::GPU_ONLY : ((memoryType == BufferMemoryType::CPU_TO_GPU) ? DEVICE_MEMORY_PROPERTY::CPU_TO_GPU : DEVICE_MEMORY_PROPERTY::CPU_ONLY);
	desc.m_size = size;
	desc.m_usage = memoryType == BufferMemoryType::GPU_ONLY ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
	if (type & BUFFER_TYPE_VERTEX)
		desc.m_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type & BUFFER_TYPE_INDEX)
		desc.m_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type & BUFFER_TYPE_UNIFORM)
		desc.m_usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (type & BUFFER_TYPE_STORAGE)
		desc.m_usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type & BUFFER_TYPE_INDIRECT)
		desc.m_usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type & BUFER_TYPE_TRANSFER_SRC)
		desc.m_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (type & BUFFER_TYPE_TRANSFER_DST)
		desc.m_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (pointerUsage)
		desc.m_usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	desc.mRequireCoherent = requireCoherent;
	IBuffer2 buffer = new GPUBuffer2();
	buffer->mContext = gContext;
	buffer->mMemoryType = memoryType;
	buffer->mSize = size;
	buffer->mType = type;
	VkAlloc::CreateBuffers(gContext->m_future_memory_context, 1, &desc, &buffer->mVkAllocBuffer);
	std::string userData = "BUFFEr";
	vmaSetAllocationUserData(gContext->m_future_memory_context->m_allocator, buffer->mVkAllocBuffer->m_suballocation.m_allocation, userData.data());
	buffer->mBuffers[0] = buffer->mVkAllocBuffer->m_buffer;
	if (!intermediateBuffer) {
		for (int i = 1; i < gFrameOverlapCount; i++) {
			VkBufferCreateInfo createInfo;
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.size = desc.m_size;
			createInfo.usage = desc.m_usage;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
			createInfo.flags = 0;
			vkCreateBuffer(gContext->defaultDevice, &createInfo, nullptr, &buffer->mBuffers[i]);
			auto& suballoc = buffer->mVkAllocBuffer->m_suballocation;
			vkBindBufferMemory(gContext->defaultDevice, buffer->mBuffers[i], suballoc.m_allocation_info.deviceMemory, suballoc.m_allocation_info.offset);
		}
	}
	return buffer;
}

IBuffer2 Buffer2_CreatePreInitalized(BufferType type, void* pData, size_t size, BufferMemoryType memoryType, bool pointerUsage, bool requireCoherent, bool intermediateBuffer)
{
	IBuffer2 buffer = Buffer2_Create(type, size, memoryType, pointerUsage, requireCoherent, intermediateBuffer);
	Buffer2_UploadData(buffer, (char8_t*)pData, 0, VK_WHOLE_SIZE);
	return buffer;
}

void Buffer2_UploadData(IBuffer2 buffer, void *pData, size_t offset, size_t size)
{
	if (size == VK_WHOLE_SIZE) {
		size = buffer->mSize - offset;
	}
	if (buffer->mMemoryType != BufferMemoryType::GPU_ONLY)
	{
		void *ptr = Buffer2_Map(buffer);
		memcpy(ptr, pData, size);
		Buffer2_Unmap(buffer);
		return;
	}
	IBuffer2 intermediate = Buffer2_Create(BUFER_TYPE_TRANSFER_SRC, size, BufferMemoryType::CPU_ONLY, false, false, true);
	Buffer2_UploadData(intermediate, pData, 0, size);
	VkFence fence = vk::Gfx_CreateFence(gContext, false);
	VkCommandPool pool = vk::Gfx_CreateCommandPool(gContext, true, false);
	VkCommandBuffer cmd = vk::Gfx_AllocCommandBuffer(gContext, pool, true);
	vk::Gfx_StartCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VkBufferCopy region;
	region.srcOffset = 0;
	region.size = size;
	region.dstOffset = offset;
	vkCmdCopyBuffer(cmd, intermediate->mBuffers[0], buffer->mBuffers[0], 1, &region);
	vkEndCommandBuffer(cmd);
	vk::Gfx_SubmitCmdBuffers(gContext->defaultQueue, {cmd}, {}, {}, {}, fence);
	vkWaitForFences(gContext->defaultDevice, 1, &fence, true, 5e9);
	Buffer2_Destroy(intermediate);
	vkDestroyFence(gContext->defaultDevice, fence, gContext->m_allocation_callback);
	vkDestroyCommandPool(gContext->defaultDevice, pool, gContext->m_allocation_callback);
}

void* Buffer2_Map(IBuffer2 buffer)
{
	assert(buffer->mMemoryType != BufferMemoryType::GPU_ONLY && "Cannot be static.");
	if (buffer->mIsMapped)
		return buffer->mMappedPtr;
	char8_t **mapped_pointer = (char8_t**)&buffer->mVkAllocBuffer->m_suballocation.m_allocation_info.pMappedData;
	VkAlloc::MapBuffer(gContext->m_future_memory_context, buffer->mVkAllocBuffer);
	buffer->mIsMapped = true;
	buffer->mMappedPtr = *mapped_pointer;
	return *mapped_pointer;
}

void Buffer2_Flush(IBuffer2 buffer, uint64_t offset, uint64_t size)
{
	if (size == VK_WHOLE_SIZE)
	{
		size = buffer->mSize - offset;
	}
	VkAlloc::FlushBuffer(gContext->m_future_memory_context, buffer->mVkAllocBuffer, offset, size);
}

void Buffer2_Invalidate(IBuffer2 buffer, uint64_t offset, uint64_t size)
{
	if (size == VK_WHOLE_SIZE)
	{
		size = buffer->mSize - offset;
	}
	VkAlloc::InvalidateBuffer(gContext->m_future_memory_context, buffer->mVkAllocBuffer, offset, size);
}

void Buffer2_Unmap(IBuffer2 buffer)
{
	assert(buffer->mMemoryType != BufferMemoryType::GPU_ONLY && "Cannot be static.");
	buffer->mIsMapped = false;
	buffer->mMappedPtr = nullptr;
	VkAlloc::UnmapBuffer(gContext->m_future_memory_context, buffer->mVkAllocBuffer);
}

uint64_t Buffer2_GetGPUPointer(IBuffer2 buffer)
{
	if (buffer->mGPUPointer != 0)
		return buffer->mGPUPointer;
	VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	info.buffer = buffer->mBuffers[0];
	uint64_t ptr = vkGetBufferDeviceAddress(gContext->defaultDevice, &info);
	if (ptr == uint64_t(NULL)) {
		logerror("Could not get GPU buffer pointer!");
	}
	buffer->mGPUPointer = ptr;
	return ptr;
}

void Buffer2_Destroy(IBuffer2 buffer)
{
	if (buffer->mIsMapped)
		Buffer2_Unmap(buffer);
	VkAlloc::DestroyBuffers(gContext->m_future_memory_context, 1, &buffer->mVkAllocBuffer);
	for (int i = 1; i < gFrameOverlapCount; i++)
		vkDestroyBuffer(gContext->defaultDevice, buffer->mBuffers[i], nullptr);
	delete buffer;
}

static std::unordered_map<uint64_t, IBuffer2> GmallocBuffers;

void* Gmalloc(uint32_t size, BufferType type) {
	IBuffer2 buffer = Buffer2_Create(type, size, BufferMemoryType::CPU_TO_GPU, true, true);
	void* ptr = Buffer2_Map(buffer);
	GmallocBuffers.insert(std::make_pair(uint64_t(ptr), buffer));
	return ptr;
}

void Gfree(void* ptr) {
	uint64_t ptr64 = uint64_t(ptr);
	if (GmallocBuffers.find(ptr64) != GmallocBuffers.end()) {
		Buffer2_Destroy(GmallocBuffers[ptr64]);
	}
}

IBuffer2 Gbuffer(void* ptr)
{
	uint64_t ptr64 = uint64_t(ptr);
	if (GmallocBuffers.find(ptr64) != GmallocBuffers.end()) {
		return GmallocBuffers[ptr64];
	}
	return nullptr;
}
