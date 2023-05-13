#pragma once
#include "egx.hpp"

namespace egx {

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
			m_Ctx->Device.destroyFence(m_Fence);
			m_Ctx->Device.destroyCommandPool(m_Pool);
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
