#pragma once
#include "fence.hpp"
#include "semaphore.hpp"
#include <optional>

namespace egx
{
	class ISynchronization {
	public:
		ISynchronization(const ref<VulkanCoreInterface>& coreInterface, const std::string& implementersClassName)
			: _CoreInterface(coreInterface), _ClassName(implementersClassName) {}

		// On the first call this function creates a semaphore
		// The semaphore is used to signal when this Synchronization object
		// has finished the executing on the GPU Side
		EGX_API virtual ref<Semaphore>& GetOrCreateCompletionSemaphore();

		EGX_API virtual bool HasCompletionSemaphore() const { return _SignalSemaphore.IsValidRef(); }

		/// <summary>
		// Tells the "this" synchronization object to wait for the following
		// object to finish executing. Wait stage tells "this" object when it can start executing
		// therefore you can control until which pipeline stage "this" object has to wait before executing.
		/// </summary>
		virtual void AddWaitObject(ISynchronization* waitObject, VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) {
			_WaitSemaphores.push_back(waitObject->GetOrCreateCompletionSemaphore());
			_WaitStageFlags.push_back(waitStage);
		}

	protected:
		EGX_API virtual void Submit(VkCommandBuffer cmd);

		[[nodiscard]] bool HasCompletionFence() const { return _Completion.IsValidRef(); }

		ref<Fence>& GetOrCreateCompletionFence() {
			if (_Completion.IsValidRef()) return _Completion;
			_Completion = Fence::FactoryCreate(_CoreInterface, true);
			return _Completion;
		}

		void WaitAndReset() {
			assert(_Completion.IsValidRef() && "Call GetOrCreateCompletionFence() to create fence.");
			_Completion->Wait(UINT64_MAX);
			_Completion->Reset();
		}

		void InternalAddWaitSemaphore(const ref<Semaphore>& waitSemaphore, VkPipelineStageFlags stageFlags)
		{
			_WaitSemaphores.push_back(waitSemaphore);
			_WaitStageFlags.push_back(stageFlags);
		}

	protected:
		ref<VulkanCoreInterface> _CoreInterface;
		std::vector<egx::ref<Semaphore>> _WaitSemaphores;
		std::vector<VkPipelineStageFlags> _WaitStageFlags;
		std::string _ClassName;

	private:
		ref<Semaphore> _SignalSemaphore;
		ref<Fence> _Completion;
	};

	//class Synchronization
	//{
	//public:
	//    Synchronization(const ref<VulkanCoreInterface>& CoreInterface) : _CoreInterface(CoreInterface) {}

	//    void SetPredecessors(const std::initializer_list<Synchronization*>& predecessors)
	//    {
	//        for (Synchronization* item : predecessors)
	//        {
	//            item->SetSuccessor(*this);
	//        }
	//    }

	//    // Can only have one successor (not timeline semaphores)
	//    void SetSuccessor(Synchronization& Successor, VkPipelineStageFlags stageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
	//    {
	//        auto& signal = GetOrCreateSignalSemaphore();
	//        Successor.SyncObjAddClientWaitSemaphore(signal, stageFlag);
	//    }

	//protected:
	//    // Must be used this frame or pointers may become invalid.
	//    VkResult Submit(VkCommandBuffer cmd)
	//    {
	//        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	//        submitInfo.waitSemaphoreCount = (uint32_t)_WaitSemaphores.size();
	//        submitInfo.pWaitSemaphores = GetWaitSemaphores().data();
	//        submitInfo.pWaitDstStageMask = _WaitStageFlags.data();
	//        submitInfo.commandBufferCount = 1;
	//        submitInfo.pCommandBuffers = &cmd;
	//        if (_SignalSemaphore.IsValidRef()) {
	//            submitInfo.signalSemaphoreCount = 1;
	//            submitInfo.pSignalSemaphores = &_SignalSemaphore->GetSemaphore();
	//        }
	//        return vkQueueSubmit(_CoreInterface->Queue, 1, &submitInfo, nullptr);
	//    }

	//    [[nodiscard]] RefSemaphore& GetOrCreateSignalSemaphore()
	//    {
	//        if (_SignalSemaphore.IsValidRef())
	//            return _SignalSemaphore;
	//        _SignalSemaphore = Semaphore::FactoryCreate(_CoreInterface, "SynchronizationBase-SignalSemaphore");
	//        return _SignalSemaphore;
	//    }

	//    void SyncObjAddClientWaitSemaphore(const RefSemaphore& semaphore, VkPipelineStageFlags stageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
	//    {
	//        _WaitSemaphores.push_back(semaphore);
	//        _WaitStageFlags.push_back(stageFlag);
	//    }

	//    std::vector<VkSemaphore>& GetWaitSemaphores() {
	//        return Semaphore::GetSemaphores(_WaitSemaphores, _WaitSemaphoreVk);
	//    }

	//protected:
	//    std::vector<RefSemaphore> _WaitSemaphores;
	//    std::vector<VkPipelineStageFlags> _WaitStageFlags;
	//    RefSemaphore _SignalSemaphore;
	//    ref<VulkanCoreInterface> _CoreInterface;
	//private:
	//    std::vector<VkSemaphore> _WaitSemaphoreVk;
	//};

}