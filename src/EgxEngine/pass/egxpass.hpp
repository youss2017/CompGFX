#pragma once
#include "../core/egxcommon.hpp"
#include "../core/egxutil.hpp"
#include "../core/pipeline/egxpipelinestate.hpp"

namespace egx {

	class Pass {

	public:
		Pass(const ref<VulkanCoreInterface>& CoreInterface, const egx::PipelineState& State, bool FrameFlightMode, bool MemoryShortLived, bool EnableIndividualReset) :
			CoreInterface(CoreInterface), State(State)
		{
			CmdPool = CreateCommandPool(CoreInterface, FrameFlightMode, MemoryShortLived, EnableIndividualReset, false);
			if (FrameFlightMode) {
				Cmd = CmdPool->AllocateBufferFrameFlightMode(true);
				FinishedSemaphore = CreateSemaphore(CoreInterface, true);
			}
			else {
				Cmd.push_back(CmdPool->AllocateBuffer(true));
				FinishedSemaphore = CreateSemaphore(CoreInterface, false);
			}
		}
		Pass(Pass&) = delete;
		virtual Pass& operator=(Pass&) = 0;

		virtual void OnStartRecording(uint32_t FrameIndex) {
			if (Cmd.size() > 1) {
				StartCommandBuffer(Cmd[FrameIndex], 0);
			}
			else {
				StartCommandBuffer(Cmd[0], 0);
			}
		}

		virtual VkCommandBuffer& OnRecord(uint32_t FrameIndex) = 0;

		virtual void OnStopRecording(uint32_t FrameIndex) {
			if (Cmd.size() > 1)
				vkEndCommandBuffer(Cmd[FrameIndex]);
			else
				vkEndCommandBuffer(Cmd[0]);
		}

	protected:
		friend class PassDispatcher;
		ref<VulkanCoreInterface> CoreInterface;
		ref<CommandPool> CmdPool;
		std::vector<VkCommandBuffer> Cmd;
		ref<Semaphore> FinishedSemaphore;
		PipelineState State;
	};

	class PassDispatcher {

	public:
		EGX_API PassDispatcher(const ref<VulkanCoreInterface>& CoreInterface);
		PassDispatcher(PassDispatcher&) = delete;
		EGX_API PassDispatcher(PassDispatcher&&) noexcept;
		EGX_API PassDispatcher& operator=(PassDispatcher&) noexcept;
		EGX_API ~PassDispatcher();

		// Returns FinishedSemaphore for each Pass
		EGX_API std::vector<VkSemaphore> ExecutePass(const std::vector<Pass*>& pass, const std::vector<ref<Semaphore>>& WaitSemaphores, const std::vector<ref<Semaphore>>& SignalSemaphores, VkFence SignalFence = nullptr);

	private:
		ref<VulkanCoreInterface> CoreInterface;
	};

}
