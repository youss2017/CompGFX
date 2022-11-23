#pragma once
#include "fence.hpp"
#include "semaphore.hpp"
#include <optional>

namespace egx
{
    using RefSemaphore = ref<Semaphore>;

    class Synchronization
    {
    public:
        Synchronization(const ref<VulkanCoreInterface>& CoreInterface) : _CoreInterface(CoreInterface) {}

        void SetPredecessors(const std::initializer_list<Synchronization*>& predecessors)
        {
            for (Synchronization* item : predecessors)
            {
                item->SetSuccessor(*this);
            }
        }

        // Can only have one successor (not timeline semaphores)
        void SetSuccessor(Synchronization& Successor, VkPipelineStageFlags stageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        {
            auto& signal = GetOrCreateSignalSemaphore();
            Successor.SyncObjAddClientWaitSemaphore(signal, stageFlag);
        }

    protected:
        // Must be used this frame or pointers may become invalid.
        VkResult Submit(VkCommandBuffer cmd)
        {
            VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.waitSemaphoreCount = (uint32_t)_WaitSemaphores.size();
            submitInfo.pWaitSemaphores = GetWaitSemaphores().data();
            submitInfo.pWaitDstStageMask = _WaitStageFlags.data();
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;
            if (_SignalSemaphore.IsValidRef()) {
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &_SignalSemaphore->GetSemaphore();
            }
            return vkQueueSubmit(_CoreInterface->Queue, 1, &submitInfo, nullptr);
        }

        [[nodiscard]] RefSemaphore& GetOrCreateSignalSemaphore()
        {
            if (_SignalSemaphore.IsValidRef())
                return _SignalSemaphore;
            _SignalSemaphore = Semaphore::FactoryCreate(_CoreInterface, "SynchronizationBase-SignalSemaphore");
            return _SignalSemaphore;
        }

        void SyncObjAddClientWaitSemaphore(const RefSemaphore& semaphore, VkPipelineStageFlags stageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        {
            _WaitSemaphores.push_back(semaphore);
            _WaitStageFlags.push_back(stageFlag);
        }

        std::vector<VkSemaphore>& GetWaitSemaphores() {
            return Semaphore::GetSemaphores(_WaitSemaphores, _WaitSemaphoreVk);
        }

    protected:
        std::vector<RefSemaphore> _WaitSemaphores;
        std::vector<VkPipelineStageFlags> _WaitStageFlags;
        RefSemaphore _SignalSemaphore;
        ref<VulkanCoreInterface> _CoreInterface;
    private:
        std::vector<VkSemaphore> _WaitSemaphoreVk;
    };


}