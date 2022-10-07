#pragma once
#include "egxpass.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace egx {

	class Scene {

	public:
		Scene(const ref<VulkanCoreInterface>& CoreInterface) :
			CoreInterface(CoreInterface), Framebuffer(InitalizeFramebuffer())
		{}

		Scene(Scene&) = delete;

		Scene(Scene&& move) noexcept {
			memcpy(this, &move, move.size());
			memset(&move, 0, move.size());
		}

		template<class T >
		void AddPass() {
			std::shared_ptr<Pass> x = std::make_shared<T>(this);
			Passes.push_back(x);
		}

		virtual std::vector<VkSemaphore> Execute(const std::vector<ref<Semaphore>>& WaitSemaphores, const std::vector<ref<Semaphore>>& SignalSemaphores, VkFence SignalFence = nullptr)
		{
			uint32_t FrameIndex = CoreInterface->CurrentFrame;

			VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
			VkSemaphore* pWaitSemaphores = (VkSemaphore*)alloca(WaitSemaphores.size() * sizeof(VkSemaphore));
			VkSemaphore* pSignalSemaphores = (VkSemaphore*)alloca((SignalSemaphores.size() + Passes.size()) * sizeof(VkSemaphore));
			VkCommandBuffer* pCommandBuffers = (VkCommandBuffer*)alloca(Passes.size() * sizeof(VkCommandBuffer));
			std::vector<VkSemaphore> FinishedSemaphores(Passes.size());

			for (int j = 0; j < WaitSemaphores.size(); j++) {
				pWaitSemaphores[j] = WaitSemaphores[j]->GetSemaphore();
			}

			for (int j = 0, k = 0; j < SignalSemaphores.size() + Passes.size(); j++) {
				if (j >= SignalSemaphores.size()) {
					auto semaphore = Passes[k]->FinishedSemaphore->GetSemaphore();
					pSignalSemaphores[j] = semaphore;
					FinishedSemaphores[k] = semaphore;
					k++;
				}
				else
					pSignalSemaphores[j] = SignalSemaphores[j]->GetSemaphore();
			}

			for (int j = 0; j < Passes.size(); j++) {
				Passes[j]->OnStartRecording(FrameIndex);
				pCommandBuffers[j] = Passes[j]->GetCommandBuffer(FrameIndex);
				Passes[j]->OnStopRecording(FrameIndex);
			}
			submitInfo.waitSemaphoreCount = (uint32_t)WaitSemaphores.size();
			submitInfo.pWaitSemaphores = pWaitSemaphores;
			submitInfo.pWaitDstStageMask = nullptr;
			submitInfo.commandBufferCount = (uint32_t)Passes.size();
			submitInfo.pCommandBuffers = pCommandBuffers;
			submitInfo.signalSemaphoreCount = (uint32_t)(SignalSemaphores.size() + Passes.size());
			submitInfo.pSignalSemaphores = pSignalSemaphores;
			vkQueueSubmit(CoreInterface->Queue, 1, &submitInfo, SignalFence);
			return FinishedSemaphores;
		}

		template<typename T>
		void RegisterSubPass(uint32_t PassId) {
			Subpasses.push_back(std::make_unique<T>(CoreInterface, Framebuffer, PassId));
		}

	protected:
		virtual size_t size() const = 0;
		EGX_API virtual egx::ref<egx::Framebuffer> InitalizeFramebuffer() = 0;

	protected:
		friend class Pass;
		ref<VulkanCoreInterface> CoreInterface;
		std::vector<std::shared_ptr<egx::Pass>> Passes;
		egx::ref<Framebuffer> Framebuffer;
		std::list<std::unique_ptr<Pass>> Subpasses;
	};

}
