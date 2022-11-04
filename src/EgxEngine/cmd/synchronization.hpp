#pragma once
#include "fence.hpp"
#include "semaphore.hpp"
#include <optional>

namespace egx
{

    using RefSemaphore = ref<Semaphore>;

    class SynchronizationContext
    {
    public:
        SynchronizationContext &AddWaitSemaphore(const RefSemaphore &semaphore, VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        {
            _producer_wait.push_back(semaphore);
            _producer_wait_stage_flags.push_back(stageMask);
            return *this;
        }

        SynchronizationContext &AddSignalSemaphore(const RefSemaphore &semaphore)
        {
            _producer_signal.push_back(semaphore);
            return *this;
        }

        const RefSemaphore &GetConsumerWaitSemaphore() const { return _consumer_wait; }

        std::vector<VkSemaphore> GetProducerWaitSemaphores(uint32_t CustomFrame = UINT32_MAX) const { return Semaphore::GetSemaphores(_producer_wait, CustomFrame); }
        std::vector<VkSemaphore> GetProducerSignalSemaphores(uint32_t CustomFrame = UINT32_MAX) const { return Semaphore::GetSemaphores(_producer_signal, CustomFrame); }
        
        const std::vector<VkPipelineStageFlags>& GetProducerWaitStageFlags() const { return _producer_wait_stage_flags; }

        void SetConsumerWaitSemaphore(const RefSemaphore& semaphore) { _consumer_wait = semaphore; }

    private:
        std::vector<RefSemaphore> _producer_wait;
        std::vector<VkPipelineStageFlags> _producer_wait_stage_flags;
        std::vector<RefSemaphore> _producer_signal;
        RefSemaphore _consumer_wait;
    };

    // /*
    //     SynchronizationCollection:
    //         For an enclosed object such as swapchain, the consumer sync object
    //         is used for wait
    // */

    // // This is what the producer waits/signals
    // class ConsumerSynchronization
    // {

    // public:
    //     const std::vector<VkSemaphore> GetWaitSemaphores() const
    //     {
    //         Semaphore::GetSemaphores(_ref_wait_semaphores);
    //     }

    //     const std::vector<VkPipelineStageFlags> GetWaitDstStageFlags() const
    //     {
    //         return _wait_dst_stages;
    //     }

    //     const std::vector<VkSemaphore> GetSignalSemaphores() const
    //     {
    //         Semaphore::GetSemaphores(_ref_signal_semaphores);
    //     }

    //     ConsumerSynchronization &AddWaitSemaphore(const RefSemaphore &semaphore, VkPipelineStageFlags WaitDstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
    //     {
    //         _ref_wait_semaphores.push_back(semaphore);
    //         _wait_dst_stages.push_back(WaitDstStage);
    //         _wait_semaphores = Semaphore::GetSemaphores(_ref_wait_semaphores);
    //         return *this;
    //     }

    //     ConsumerSynchronization &AddSignalSemaphore(const RefSemaphore &semaphore)
    //     {
    //         _ref_signal_semaphores.push_back(semaphore);
    //         _signal_semaphores = Semaphore::GetSemaphores(_ref_signal_semaphores);
    //         return *this;
    //     }

    //     VkSemaphore GetNamedWaitSemaphore(const std::string &Name) { return Semaphore::GetSemaphoreFromName(Name, _ref_wait_semaphores)->GetSemaphore(); }
    //     VkSemaphore GetNamedSignalSemaphore(const std::string &Name) { return Semaphore::GetSemaphoreFromName(Name, _ref_signal_semaphores)->GetSemaphore(); }

    //     const RefSemaphore &GetNamedRefWaitSemaphore(const std::string &Name) { return Semaphore::GetSemaphoreFromName(Name, _ref_wait_semaphores); }
    //     const RefSemaphore &GetNamedRefSignalSemaphore(const std::string &Name) { return Semaphore::GetSemaphoreFromName(Name, _ref_signal_semaphores); }

    // private:
    //     std::vector<RefSemaphore> _ref_wait_semaphores;
    //     std::vector<VkPipelineStageFlags> _wait_dst_stages;
    //     std::vector<RefSemaphore> _ref_signal_semaphores;
    //     std::vector<VkSemaphore> _wait_semaphores;
    //     std::vector<VkSemaphore> _signal_semaphores;
    // };

    // // This is what you wait on
    // class ProducerSynchronization
    // {
    // public:
    //     const std::vector<VkSemaphore> GetWaitSemaphores() const
    //     {
    //         return _wait_semaphores;
    //     }

    //     const std::vector<VkPipelineStageFlags> GetWaitDstStageFlags() const
    //     {
    //         return _wait_dst_stages;
    //     }

    //     ProducerSynchronization &AddWaitSemaphore(const RefSemaphore &semaphore, VkPipelineStageFlags WaitDstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
    //     {
    //         _ref_wait_semaphores.push_back(semaphore);
    //         _wait_dst_stages.push_back(WaitDstStage);
    //         _wait_semaphores = Semaphore::GetSemaphores(_ref_wait_semaphores);
    //         return *this;
    //     }

    //     VkSemaphore GetNamedWaitSemaphore(const std::string &Name) { return Semaphore::GetSemaphoreFromName(Name, _ref_wait_semaphores)->GetSemaphore(); }
    //     const RefSemaphore &GetNamedRefWaitSemaphore(const std::string &Name) { return Semaphore::GetSemaphoreFromName(Name, _ref_wait_semaphores); }

    // private:
    //     std::vector<RefSemaphore> _ref_wait_semaphores;
    //     std::vector<VkPipelineStageFlags> _wait_dst_stages;
    //     std::vector<VkSemaphore> _wait_semaphores;
    // };

    // class SynchronizationCollection
    // {
    // public:
    //     void SetSignalFence(const ref<Fence> &fence)
    //     {
    //         _signal_fence = fence;
    //     }

    //     ref<Fence> &GetSignalFence() { return _signal_fence; }

    // public:
    //     ConsumerSynchronization ConsumerSync;
    //     ProducerSynchronization ProducerSync;

    // private:
    //     ref<Fence> _signal_fence;
    // };

}