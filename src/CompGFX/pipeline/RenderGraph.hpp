#pragma once
#include <core/egx.hpp>
#include <memory/egximage.hpp>
#include "pipeline.hpp"
#include "shaders/shader.hpp"
#include <functional>

namespace egx
{

	class ResourceDescriptorPool {
	public:
		ResourceDescriptorPool() = default;
		ResourceDescriptorPool(const DeviceCtx& pCtx);
		~ResourceDescriptorPool();
		vk::DescriptorPool GetPool() const { return *m_Pool; }
	private:
		DeviceCtx m_Ctx;
		std::shared_ptr<vk::DescriptorPool> m_Pool;
	};

	class ResourceDescriptor
	{
	public:
		ResourceDescriptor() = default;
		ResourceDescriptor(const DeviceCtx& pCtx, const ResourceDescriptorPool& pool, const PipelineType& pipeline);

		ResourceDescriptor& SetInput(int setId, int bindingId, const Buffer& particle_buffer);
		ResourceDescriptor& SetInput(int setId, int bindingId, vk::ImageLayout layout, int viewId, const Image2D& image, vk::Sampler sampler = {});

		ResourceDescriptor& SetInput(int bindingId, const Buffer& particle_buffer) { return SetInput(0, bindingId, particle_buffer); }
		ResourceDescriptor& SetInput(int bindingId, vk::ImageLayout layout, int viewId, const Image2D& image, vk::Sampler sampler = {})
		{
			return SetInput(0, bindingId, layout, viewId, image, sampler);
		}

		void SetBufferOffset(int setId, int bindingId, uint32_t offset);
		void SetBufferOffset(int bindingId, uint32_t offset) { SetBufferOffset(0, bindingId, offset); }

		void Bind(vk::CommandBuffer cmd);

	private:
		struct DataWrapper
		{
			DeviceCtx m_Ctx;
			ResourceDescriptorPool m_Pool;
			std::unique_ptr<PipelineType> m_Pipeline;
			// [frame, [set id, set]]
			std::map<uint32_t, std::map<uint32_t, vk::DescriptorSet>> m_Sets;
			std::map<uint32_t, std::map<uint32_t, uint32_t>> m_OffsetMapping;
			std::vector<uint32_t> m_Offsets;

			DataWrapper() = default;
			DataWrapper(DataWrapper&) = delete;
			~DataWrapper();
		};

	private:
		std::shared_ptr<DataWrapper> m_Data;
		ShaderReflection m_Reflection;
	};

	/*
		Features:
		1) Allow for prebaked command buffers
			- Also allow the possability for seperate buffers to allow for static and dynamic buffers
		2) Manage Dependencies between stages
		3) Integrate RenderGraph with other graphs
		4) Integrate RenderGraph with swapchain
	*/

	class GraphSynchronization
	{
	public:
		GraphSynchronization& Add(const Buffer& particle_buffer, vk::AccessFlags2 srcAccess,
			vk::AccessFlags2 dstAccess, vk::PipelineStageFlags2 srcStage,
			vk::PipelineStageFlags2 dstStage)
		{
			vk::BufferMemoryBarrier2 barrier;
			barrier.setDstAccessMask(dstAccess)
				.setSrcAccessMask(srcAccess)
				.setSrcStageMask(srcStage)
				.setDstStageMask(dstStage)
				.setSize(particle_buffer.Size());
			m_BufferBarriers.push_back(barrier);
			m_Buffers.push_back(particle_buffer);
			return *this;
		}

		GraphSynchronization& Add(const Image2D& image, vk::AccessFlags2 srcAccess,
			vk::AccessFlags2 dstAccess, vk::PipelineStageFlags2 srcStage,
			vk::PipelineStageFlags2 dstStage)
		{
			vk::ImageMemoryBarrier2 barrier;
			barrier.setDstAccessMask(dstAccess)
				.setSrcAccessMask(srcAccess)
				.setSrcStageMask(srcStage)
				.setDstStageMask(dstStage)
				.setImage(image.GetHandle());
			m_ImageBarriers.push_back(barrier);
			return *this;
		}

		const vk::DependencyInfo& ReadDependencyInfo()
		{
			for (auto i = 0ull; i < m_BufferBarriers.size(); i++)
			{
				m_BufferBarriers[i].setBuffer(m_Buffers[i].GetHandle());
			}
			m_Dependency.setBufferMemoryBarriers(m_BufferBarriers).setImageMemoryBarriers(m_ImageBarriers);
			return m_Dependency;
		}

		size_t BarrierCount() const { return m_BufferBarriers.size() + m_ImageBarriers.size(); }

	private:
		std::vector<Buffer> m_Buffers;
		std::vector<vk::BufferMemoryBarrier2> m_BufferBarriers;
		std::vector<vk::ImageMemoryBarrier2> m_ImageBarriers;
		vk::DependencyInfo m_Dependency;
	};

	class RenderGraph
	{
	public:
		RenderGraph(const DeviceCtx& ctx);

		RenderGraph& Add(const PipelineType& pipeline,
			const std::function<void(vk::CommandBuffer)>& stageCallback,
			const GraphSynchronization& synchronization = {});

		ResourceDescriptor CreateResourceDescriptor(const PipelineType& pipeline);
		vk::Fence RunAsync();
		void Run();

		RenderGraph& AddWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlags stageFlag);
		RenderGraph& UseSignalSemaphore();
		vk::Semaphore GetCompletionSemaphore() const
		{
			if (VkSemaphore(m_Data->m_CompletionSemaphore) == nullptr)
			{
				throw std::runtime_error("To call GetCompletionSemaphore() you must first call UseSignalSemaphore()");
				return nullptr;
			}
			return m_Data->m_CompletionSemaphore;
		}

	private:
		struct Stage
		{
			std::unique_ptr<PipelineType> Pipeline;
			std::function<void(vk::CommandBuffer)> Callback;
			GraphSynchronization Synchronization;
		};

		struct DataWrapper
		{
			DeviceCtx m_Ctx;
			vk::DescriptorPool m_DescriptorPool;
			std::map<uint32_t, ResourceDescriptor> m_Descriptors;
			std::vector<Stage> m_Stages;
			std::vector<std::tuple<vk::CommandPool, vk::CommandBuffer, vk::Fence>> m_Cmds;
			vk::Semaphore m_CompletionSemaphore;
			// ASSERT(length(m_WaitSemaphores) == length(m_WaitStages))
			std::vector<vk::Semaphore> m_WaitSemaphores;
			std::vector<vk::PipelineStageFlags> m_WaitStages;

			DataWrapper() = default;
			DataWrapper(DataWrapper&) = delete;
			~DataWrapper();
		};

	private:
		std::shared_ptr<DataWrapper> m_Data;
	};

}