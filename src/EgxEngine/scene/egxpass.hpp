#pragma once
#include "../core/egxcommon.hpp"
#include "../core/egxutil.hpp"
#include "../core/pipeline/egxpipelinestate.hpp"
#include <memory>

namespace egx {

	class Scene;

	class Pass {

	protected:
		Pass(const ref<VulkanCoreInterface>& CoreInterface, const ref<egx::Framebuffer>& Framebuffer, uint32_t PassId,
			bool FrameFlightMode = true, bool MemoryShortLived = false, bool EnableIndividualReset = false) :
			CoreInterface(CoreInterface), Framebuffer(Framebuffer), PassId(PassId),
			State(InitalizePipelineState(Framebuffer, PassId))
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

		virtual void OnStartRecording(uint32_t FrameIndex) {
			if (Cmd.size() > 1) {
				StartCommandBuffer(Cmd[FrameIndex], 0);
			}
			else {
				StartCommandBuffer(Cmd[0], 0);
			}
		}

		virtual VkCommandBuffer& GetCommandBuffer(uint32_t FrameIndex) = 0;
		virtual void OnRecordBuffer(uint32_t FrameIndex) = 0;

		virtual void OnStopRecording(uint32_t FrameIndex) {
			if (Cmd.size() > 1)
				vkEndCommandBuffer(Cmd[FrameIndex]);
			else
				vkEndCommandBuffer(Cmd[0]);
		}

		virtual std::shared_ptr<egx::PipelineState> InitalizePipelineState(const egx::ref<egx::Framebuffer>& Framebuffer, const uint32_t PassId) = 0;

	protected:
		friend class Scene;
		ref<VulkanCoreInterface> CoreInterface;
		ref<egx::Framebuffer> Framebuffer;
		const uint32_t PassId;
		std::shared_ptr<egx::PipelineState> State;
		ref<CommandPool> CmdPool;
		std::vector<VkCommandBuffer> Cmd;
		ref<Semaphore> FinishedSemaphore;
	};

}
