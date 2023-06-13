#include "egxbuffer.hpp"
#include <core/ScopedCommandBuffer.hpp>

using namespace egx;

egx::Buffer::Buffer(const DeviceCtx &pCtx, size_t size, MemoryPreset memoryPreset, HostMemoryAccess memoryAccess, vk::BufferUsageFlags usage, bool isFrameResource)
	: m_Size(size), m_MemoryAccessBehavior(memoryAccess), m_MemoryType(memoryPreset)
	, IsFrameResource(isFrameResource)
{
	usage |= vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
	Usage = usage;

	m_Data = std::make_shared<Buffer::DataWrapper>();
	m_Data->m_Ctx = pCtx;

	if (!IsFrameResource)
	{
		VkResult result = _CreateBufferVma((VkBuffer*)&m_Data->m_Buffer, &m_Data->m_Allocation);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(cpp::Format("Could create buffer with {} bytes, usage={}, error code {}", size, vk::to_string(usage), vk::to_string(vk::Result(result))));
		}
	}
	else
	{
		m_Data->m_Allocations.resize(pCtx->FramesInFlight);
		m_Data->m_MappedPtrs.resize(pCtx->FramesInFlight);
		for (uint32_t i = 0; i < pCtx->FramesInFlight; i++)
		{
			VkBuffer temp;
			VkResult result = _CreateBufferVma(&temp, &m_Data->m_Allocations[i]);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error(cpp::Format("Could create buffer with {} bytes, usage={}, error code {}", size, vk::to_string(usage), vk::to_string(vk::Result(result))));
			}
			m_Data->m_Buffers.push_back(temp);
		}
	}
}
uint8_t &Buffer::operator[](size_t index)
{
	return *(((uint8_t *)Map()) + index);
}

void Buffer::_Write(const void *pData, size_t offset, size_t size, int resourceId)
{
	// Issue potential resize
	GetHandle(resourceId);

	if (m_MemoryType != MemoryPreset::DeviceOnly)
	{
		bool previousMappedState = m_Data->m_IsMapped;
		Map();

		if (IsFrameResource)
		{
			uint8_t *pMemory = (uint8_t *)m_Data->m_MappedPtrs[resourceId];
			memcpy(pMemory + offset, pData, size);
			vmaFlushAllocation(m_Data->m_Ctx->Allocator, m_Data->m_Allocations[resourceId], offset, size);
		}
		else
		{
			uint8_t *pMemory = (uint8_t *)Map();
			memcpy(pMemory + offset, pData, size);
			vmaFlushAllocation(m_Data->m_Ctx->Allocator, m_Data->m_Allocation, offset, size);
		}
		if (!previousMappedState)
			Unmap();

		return;
	}

	ScopedCommandBuffer cmd(m_Data->m_Ctx);
	Buffer stage(m_Data->m_Ctx, size, MemoryPreset::HostOnly, HostMemoryAccess::Sequential, vk::BufferUsageFlagBits::eTransferSrc, false);
	stage[0] = 8;
	stage.Write(pData);
	stage._CopyTo(*cmd, *this, 0, offset, size, resourceId);
}

void Buffer::Write(const void *pData, size_t offset, size_t size)
{
	_Write(pData, offset, size, m_Data->m_Ctx->CurrentFrame);
}
void Buffer::Write(const void *pData)
{
	Write(pData, 0, m_Size);
}
void *Buffer::Map()
{
	if (!m_Data->m_IsMapped)
	{
		if (!IsFrameResource)
			vmaMapMemory(m_Data->m_Ctx->Allocator, m_Data->m_Allocation, &m_Data->m_MappedPtr);
		else
		{
			for (uint32_t i = 0; i < m_Data->m_Ctx->FramesInFlight; i++)
			{
				vmaMapMemory(m_Data->m_Ctx->Allocator, m_Data->m_Allocations[i], &m_Data->m_MappedPtrs[i]);
			}
		}
	}
	m_Data->m_IsMapped = true;
	if (IsFrameResource)
		return m_Data->m_MappedPtrs[m_Data->m_Ctx->CurrentFrame];
	return m_Data->m_MappedPtr;
}
void Buffer::Unmap()
{
	if (!m_Data->m_IsMapped)
		return;
	if (IsFrameResource)
	{
		for (uint32_t i = 0; i < m_Data->m_Ctx->FramesInFlight; i++)
		{
			vmaUnmapMemory(m_Data->m_Ctx->Allocator, m_Data->m_Allocations[i]);
			m_Data->m_MappedPtrs[i] = nullptr;
		}
	}
	else
	{
		m_Data->m_MappedPtr = nullptr;
		vmaUnmapMemory(m_Data->m_Ctx->Allocator, m_Data->m_Allocation);
	}
	m_Data->m_IsMapped = false;
}

void Buffer::FlushToGpu()
{
	if (IsFrameResource)
		vmaFlushAllocation(m_Data->m_Ctx->Allocator, m_Data->m_Allocations[m_Data->m_Ctx->CurrentFrame], 0, m_Size);
	else
		vmaFlushAllocation(m_Data->m_Ctx->Allocator, m_Data->m_Allocation, 0, m_Size);
}

void Buffer::InvalidateToCpu()
{
	if (IsFrameResource)
		vmaInvalidateAllocation(m_Data->m_Ctx->Allocator, m_Data->m_Allocations[m_Data->m_Ctx->CurrentFrame], 0, m_Size);
	else
		vmaInvalidateAllocation(m_Data->m_Ctx->Allocator, m_Data->m_Allocation, 0, m_Size);
}

void Buffer::WriteAll(const void *pData, size_t offset, size_t size)
{
	if (IsFrameResource)
	{
		for (uint32_t i = 0; i < m_Data->m_Ctx->FramesInFlight; i++)
		{
			_Write(pData, offset, size, i);
		}
	}
	else
		_Write(pData, offset, size, 0);
}

void Buffer::WriteAll(const void *pData)
{
	WriteAll(pData, 0, m_Size);
}

void Buffer::Read(size_t offset, size_t size, void *pOutData)
{
	if (m_MemoryType != MemoryPreset::DeviceOnly)
	{
		MemoryMappedScope mapScope(*this);
		memcpy(pOutData, mapScope.Ptr + offset, size);
		return;
	}
	ScopedCommandBuffer cmd(m_Data->m_Ctx);
	Buffer stage(m_Data->m_Ctx, size, MemoryPreset::HostOnly, HostMemoryAccess::Sequential, vk::BufferUsageFlagBits::eTransferDst, false);
	CopyTo(*cmd, stage, offset, 0, size);
	cmd.RunNow();
	memcpy(pOutData, stage.Map(), size);
}

void Buffer::Read(void *pOutData)
{
	Read(0, m_Size, pOutData);
}

bool Buffer::Resize(size_t size)
{
	if (size == m_Size)
		return false;
	m_Data->m_ResizeBufferFrameIds.clear();
	if (!IsFrameResource) {
		m_Data->m_ResizeBufferFrameIds.push_back(0);
	}
	else {
		for (uint32_t i = 0; i < m_Data->m_Allocations.size(); i++) {
			m_Data->m_ResizeBufferFrameIds.push_back(i);
		}
	}
	m_Size = size;
	return true;
}

void Buffer::_CopyTo(vk::CommandBuffer cmd, Buffer &dst, size_t srcOffset, size_t dstOffset, size_t size, int dstResourceId)
{
	cmd.copyBuffer(GetHandle(dstResourceId), dst.GetHandle(dstResourceId), vk::BufferCopy(srcOffset, dstOffset, size));
}

VkResult egx::Buffer::_CreateBufferVma(VkBuffer* pOutBuffer, VmaAllocation* pOutAllocation) const
{
	if (m_Size == 0)
	{
		throw std::invalid_argument("Size == 0, cannot create buffer with size 0");
	}
	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = m_Size;
	createInfo.usage = VkBufferUsageFlags(Usage);
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;

	switch (m_MemoryAccessBehavior)
	{
	case HostMemoryAccess::None:
		break;
	case HostMemoryAccess::Sequential:
		allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		allocCreateInfo.flags = allocCreateInfo.flags | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		break;
	case HostMemoryAccess::Random:
		allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		allocCreateInfo.flags = allocCreateInfo.flags | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		break;
	default:
		throw std::invalid_argument("HostMemoryAccess is invalid.");
	}

	switch (m_MemoryType)
	{
	case MemoryPreset::DeviceOnly:
		if (m_MemoryAccessBehavior != HostMemoryAccess::None)
		{
			throw std::invalid_argument("HostMemoryAccess must be none with DeviceOnly memory preset.");
		}
		allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		allocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case MemoryPreset::DeviceAndHost:
		allocCreateInfo.preferredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		allocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		break;
	case MemoryPreset::HostOnly:
		allocCreateInfo.preferredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		allocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		break;
	default:
		throw std::invalid_argument("MemoryPreset is invalid.");
	}

	return vmaCreateBuffer(m_Data->m_Ctx->Allocator, &createInfo, &allocCreateInfo, pOutBuffer, pOutAllocation, nullptr);
}

void Buffer::CopyTo(vk::CommandBuffer cmd, Buffer &dst, size_t srcOffset, size_t dstOffset, size_t size)
{
	_CopyTo(cmd, dst, srcOffset, dstOffset, size, -1);
}

void Buffer::CopyTo(vk::CommandBuffer cmd, Buffer &dst)
{
	CopyTo(cmd, dst, 0, 0, m_Size);
}

vk::Buffer Buffer::GetHandle(int specificFrameIndex) const
{
	uint32_t frame = specificFrameIndex < 0 ? m_Data->m_Ctx->CurrentFrame : specificFrameIndex;
	if (m_Data->m_ResizeBufferFrameIds.size() == 0) {
		return IsFrameResource ? m_Data->m_Buffers[frame] : m_Data->m_Buffer;
	}
	else {
		// Resize buffer and return handle
		if (IsFrameResource) {
			bool resize_flag = false;
			uint32_t i = 0;
			for (; i < m_Data->m_ResizeBufferFrameIds.size(); i++) {
				if (frame == m_Data->m_ResizeBufferFrameIds[i]) {
					resize_flag = true;
					break;
				}
			}

			if (resize_flag) {
				m_Data->m_ResizeBufferFrameIds.erase(m_Data->m_ResizeBufferFrameIds.begin() + i);
				vmaDestroyBuffer(m_Data->m_Ctx->Allocator, m_Data->m_Buffers[frame], m_Data->m_Allocations[frame]);
				auto result = _CreateBufferVma((VkBuffer*)&m_Data->m_Buffers[frame], &m_Data->m_Allocations[frame]);
				if (result != VK_SUCCESS)
				{
					throw std::runtime_error(cpp::Format("Could create buffer with {} bytes, usage={}, error code {}", m_Size, vk::to_string(Usage), vk::to_string(vk::Result(result))));
				}
			}

			return m_Data->m_Buffers[frame];
		}
		else {
			m_Data->m_ResizeBufferFrameIds.clear();
			vmaDestroyBuffer(m_Data->m_Ctx->Allocator, m_Data->m_Buffer, m_Data->m_Allocation);
			auto result = _CreateBufferVma((VkBuffer*)&m_Data->m_Buffer, &m_Data->m_Allocation);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error(cpp::Format("Could create buffer with {} bytes, usage={}, error code {}", m_Size, vk::to_string(Usage), vk::to_string(vk::Result(result))));
			}
			return m_Data->m_Buffer;
		}
	}
}

Buffer::DataWrapper::~DataWrapper()
{
	if (m_Buffers.size() > 0)
	{
		for (auto i = 0ull; i < m_Buffers.size(); i++)
		{
			vmaDestroyBuffer(m_Ctx->Allocator, m_Buffers[i], m_Allocations[i]);
		}
	}
	else
	{
		vmaDestroyBuffer(m_Ctx->Allocator, m_Buffer, m_Allocation);
	}
}
