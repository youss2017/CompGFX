#include "egxpass.hpp"

namespace egx {

	egx::PassDispatcher::PassDispatcher(const ref<VulkanCoreInterface>& CoreInterface) : 
		CoreInterface(CoreInterface)
	{
	}

	egx::PassDispatcher::PassDispatcher(PassDispatcher&& move) noexcept
		: CoreInterface(move.CoreInterface)
	{}

	PassDispatcher& egx::PassDispatcher::operator=(PassDispatcher& move) noexcept
	{
		memcpy(this, &move, sizeof(PassDispatcher));
		memset(&move, 0, sizeof(PassDispatcher));
		return *this;
	}

	egx::PassDispatcher::~PassDispatcher()
	{
	}

	std::vector<VkSemaphore> egx::PassDispatcher::ExecutePass(const std::vector<Pass*>& pass, const std::vector<ref<Semaphore>>& WaitSemaphores, const std::vector<ref<Semaphore>>& SignalSemaphores, VkFence SignalFence)
	{
		uint32_t FrameIndex = CoreInterface->CurrentFrame;

		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		VkSemaphore* pWaitSemaphores = (VkSemaphore*)alloca(WaitSemaphores.size() * sizeof(VkSemaphore));
		VkSemaphore* pSignalSemaphores = (VkSemaphore*)alloca((SignalSemaphores.size() + pass.size()) * sizeof(VkSemaphore));
		VkCommandBuffer* pCommandBuffers = (VkCommandBuffer*)alloca(pass.size() * sizeof(VkCommandBuffer));
		std::vector<VkSemaphore> FinishedSemaphores(pass.size());

		for (int j = 0; j < WaitSemaphores.size(); j++) {
			pWaitSemaphores[j] = WaitSemaphores[j]->GetSemaphore();
		}

		for (int j = 0, k = 0; j < SignalSemaphores.size() + pass.size(); j++) {
			if (j >= SignalSemaphores.size()) {
				auto semaphore = pass[k]->FinishedSemaphore->GetSemaphore();
				pSignalSemaphores[j] = semaphore;
				FinishedSemaphores[k] = semaphore;
				k++;
			} else
				pSignalSemaphores[j] = SignalSemaphores[j]->GetSemaphore();
		}

		for (int j = 0; j < pass.size(); j++) {
			pass[j]->OnStartRecording(FrameIndex);
			pCommandBuffers[j] = pass[j]->OnRecord(FrameIndex);
			pass[j]->OnStopRecording(FrameIndex);
		}
		submitInfo.waitSemaphoreCount = (uint32_t)WaitSemaphores.size();
		submitInfo.pWaitSemaphores = pWaitSemaphores;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = (uint32_t)pass.size();
		submitInfo.pCommandBuffers = pCommandBuffers;
		submitInfo.signalSemaphoreCount = (uint32_t)(SignalSemaphores.size() + pass.size());
		submitInfo.pSignalSemaphores = pSignalSemaphores;
		vkQueueSubmit(CoreInterface->Queue, 1, &submitInfo, SignalFence);
		return FinishedSemaphores;
	}

}
