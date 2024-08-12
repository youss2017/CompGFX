#pragma once
#include "egx.hpp"

namespace egx {


	class IFramedSemaphore {
	public:
		IFramedSemaphore() = default;
		IFramedSemaphore(const DeviceCtx& device) {
			Invalidate(device);
		}
		void Invalidate(const DeviceCtx& device);
		vk::Semaphore GetSemaphore(const std::string& name = "default");

	private:
		struct DataWrapper {
			DeviceCtx _Device;
			std::unordered_map<std::string, std::vector<vk::Semaphore>> _Semaphores;
			DataWrapper(const DeviceCtx& device);
			~DataWrapper();

			vk::Semaphore _GetSemaphore(const std::string& name);
		};
		std::shared_ptr<DataWrapper> _data;
	};

	class IFramedFence {
	public:
		IFramedFence() = default;
		IFramedFence(const DeviceCtx& device) {
			Invalidate(device);
		}
		void Invalidate(const DeviceCtx& device);
		vk::Fence GetFence(bool createSignaled, const std::string& name = "default");

		void Wait(const std::string& name);
		void Reset(const std::string& name);

	private:
		struct DataWrapper {
			DeviceCtx _Device;
			std::unordered_map<std::string, std::vector<vk::Fence>> _Fences;
			DataWrapper(const DeviceCtx& device);
			~DataWrapper();

			vk::Fence _GetFence(bool createSignaled, const std::string& name);
		};
		std::shared_ptr<DataWrapper> _data;
	};

	class ICommandBuffer
	{
	public:
		ICommandBuffer() = default;
		
		ICommandBuffer(const DeviceCtx& device) {
			Invalidate(device);
		}

		void Invalidate(const DeviceCtx& device);

		void Reset();
		vk::CommandBuffer GetCmd(const std::string& name = "default");

	private:
		struct Pool {
			vk::CommandPool _Pool;
			std::unordered_map<std::string, vk::CommandBuffer> _Cmds;
			vk::CommandBuffer GetOrCreateCmd(const DeviceCtx& ctx, const std::string& name);
		};

		struct DataWrapper {
			DeviceCtx _device;
			std::vector<Pool> _Pools;
			DataWrapper(const DeviceCtx& device);
			~DataWrapper();
			vk::CommandBuffer _GetCmd(const std::string& name);
			void _Reset();
		};
		std::shared_ptr<DataWrapper> _data;
	};

	class ScopedCommandBuffer {
	public:
		ScopedCommandBuffer(const DeviceCtx& ctx) : m_Ctx(ctx) {
			m_Pool = ctx->Device.createCommandPool({});
			m_Cmd = ctx->Device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(m_Pool, vk::CommandBufferLevel::ePrimary, 1))[0];
			m_Fence = ctx->Device.createFence({});
			m_Cmd.begin(vk::CommandBufferBeginInfo());
		}

		void RunNow() {
			RunNowAsync();
			_wait();
		}

		void RunNowAsync() {
			m_Executed = true;
			m_Cmd.end();
			m_Ctx->Queue.submit(vk::SubmitInfo({}, {}, m_Cmd), m_Fence);
		}

		vk::CommandBuffer Get() { return m_Cmd; }

		~ScopedCommandBuffer() {
			if (!m_Executed) {
				RunNow();
			}
			m_Ctx->Device.freeCommandBuffers(m_Pool, m_Cmd);
			m_Ctx->Device.destroyFence(m_Fence);
			m_Ctx->Device.destroyCommandPool(m_Pool);
		}

		void Reset() {
			m_Ctx->Device.resetCommandPool(m_Pool);
			m_Ctx->Device.resetFences(m_Fence);
			m_Cmd.begin(vk::CommandBufferBeginInfo());
		}

		vk::CommandBuffer* operator->() { return &m_Cmd; }
		vk::CommandBuffer& operator*() { return m_Cmd; }

	private:
		void _wait() {
			auto waitResult = m_Ctx->Device.waitForFences(m_Fence, true, std::numeric_limits<uint64_t>::max());
			if (waitResult != vk::Result::eSuccess) {
				throw std::runtime_error(cpp::Format("Wait Failed on Fence, Result={}", vk::to_string(waitResult)));
			}
		}

		const DeviceCtx& m_Ctx;
		vk::Fence m_Fence;
		vk::CommandPool m_Pool;
		vk::CommandBuffer m_Cmd;
		bool m_Executed = false;
	};

}