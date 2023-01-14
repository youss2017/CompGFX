#pragma once
#include "fence.hpp"
#include "semaphore.hpp"
#include <optional>

namespace egx
{
	class ISynchronization {
	public:
		ISynchronization(const ref<VulkanCoreInterface>& coreInterface, const std::string& implementersClassName)
			: _CoreInterface(coreInterface), _ClassName(implementersClassName) {
			_IsExecuting.resize(_CoreInterface->MaxFramesInFlight, false);
		}

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
			waitObject->GetOrCreateCompletionSemaphore();
			_WaitObjects.push_back(waitObject);
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

		// every frame childern of this class must call this function to declare
		// they are executing.
		void SetExecuting() { _IsExecuting[_CoreInterface->CurrentFrame] = true; }
		// tells other objects this will be executing, therefore perform synchronization
		bool IsExecuting() const { return _IsExecuting[_CoreInterface->CurrentFrame]; }
		void ResetExecution() { _IsExecuting[_CoreInterface->CurrentFrame] = false; }

	protected:
		ref<VulkanCoreInterface> _CoreInterface;
		std::vector<ISynchronization*> _WaitObjects;
		std::vector<VkPipelineStageFlags> _WaitStageFlags;
		std::string _ClassName;

	private:
		ref<Semaphore> _SignalSemaphore;
		ref<Fence> _Completion;
		std::vector<bool> _IsExecuting;
	};

}