#pragma once
#include <core/egx.hpp>

namespace egx
{
	enum class MemoryPreset
	{
		HostOnly,
		DeviceAndHost,
		DeviceOnly
	};

	enum class HostMemoryAccess
	{
		None,
		Sequential,
		Random,
		Default = Sequential,
	};

	class Buffer
	{
	public:
		Buffer(const DeviceCtx& pCtx, size_t size, MemoryPreset memoryPreset, HostMemoryAccess memoryAccess, vk::BufferUsageFlags usage, bool isFrameResource);
		Buffer() = default;
		
		~Buffer() noexcept {
			if(IsMapped()) {
				Unmap();
			}
		}

		uint8_t& operator[](size_t index);

		void Write(const void* pData, size_t offset, size_t size);
		void Write(const void* pData);
		void* Map();
		void Unmap();

		void FlushToGpu();
		void InvalidateToCpu();

		void WriteAll(const void* pData, size_t offset, size_t size);
		void WriteAll(const void* pData);

		void Read(size_t offset, size_t size, void* pOutData);
		void Read(void* pOutData);

		/// <summary>
		/// This function issues a resize commands,
		/// when GetHandle() is called, the current frame's
		/// buffer is resized.
		/// </summary>
		/// <param name="size"></param>
		/// <returns>Was the buffer resized.</returns>
		bool Resize(size_t size);

		void CopyTo(vk::CommandBuffer cmd, Buffer& dst, size_t srcOffset, size_t dstOffset, size_t size);
		void CopyTo(vk::CommandBuffer cmd, Buffer& dst);

		vk::Buffer GetHandle(int specificFrameIndex = -1, bool* pOutResizeFlag = nullptr) const;

		bool IsMapped() const { return m_Data->m_IsMapped; }
		size_t Size() const { return m_Size; }

		template<typename T>
		static Buffer CreateFromVector(const DeviceCtx& device, const std::vector<T>& data, MemoryPreset memoryPreset, HostMemoryAccess memoryAccess, vk::BufferUsageFlags usage, bool isFrameResource) {
			Buffer result(device, data.size() * sizeof(T), memoryPreset, memoryAccess, usage, isFrameResource);
			result.WriteAll(data.data());
			return result;
		}

	public:
		vk::BufferUsageFlags Usage;
		bool IsFrameResource;

	private:
		void _Write(const void* pData, size_t offset, size_t size, int resourceId);
		void _CopyTo(vk::CommandBuffer cmd, Buffer& dst, size_t srcOffset, size_t dstOffset, size_t size, int dstResourceId);
		VkResult _CreateBufferVma(VkBuffer* pOutBuffer, VmaAllocation* pOutAllocation) const;

		struct DataWrapper
		{
			DeviceCtx m_Ctx;
			mutable std::vector<uint32_t> m_ResizeBufferFrameIds;

			mutable vk::Buffer m_Buffer;
			mutable std::vector<vk::Buffer> m_Buffers;
			
			void* m_MappedPtr = nullptr;
			std::vector<void*> m_MappedPtrs;

			mutable VmaAllocation m_Allocation = nullptr;
			mutable std::vector<VmaAllocation> m_Allocations;

			bool m_IsMapped = false;

			DataWrapper() = default;
			DataWrapper(DataWrapper&) = delete;
			~DataWrapper();
		};

	private:
		size_t m_Size;
		VmaMemoryUsage m_MemoryUsage;

		MemoryPreset m_MemoryType;
		HostMemoryAccess m_MemoryAccessBehavior;

		std::shared_ptr<DataWrapper> m_Data;
	};

	template <class T>
	struct MemoryMappedScope
	{

		uint8_t* Ptr;
		bool _CurrentMapState;
		T& _Resource;

		MemoryMappedScope(T& resource) : _Resource(resource)
		{
			_CurrentMapState = resource.IsMapped();
			Ptr = (uint8_t*)resource.Map();
		}

		~MemoryMappedScope()
		{
			_Resource.FlushToGpu();
			if (!_CurrentMapState)
			{
				_Resource.Unmap();
			}
		}
	};

}
