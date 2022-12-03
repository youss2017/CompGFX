#pragma once
#include "../core/egxcommon.hpp"
#include "../core/memory/frameflight.hpp"
#include <vector>

namespace egx {

    class Fence : public FrameFlight {
    public:
        EGX_API Fence() = default;
        EGX_API Fence(const ref<VulkanCoreInterface>& CoreInterface, bool CreateSignaled = false);
        EGX_API Fence(Fence&&) noexcept;
        EGX_API ~Fence();
        EGX_API void DelayInitalize(const ref<VulkanCoreInterface>& CoreInterface, bool CreateSignaled = false);
        EGX_API const VkFence& GetFence();
        EGX_API bool Wait(uint64_t TimeoutNanoseconds);
        EGX_API void Reset();

        EGX_API static bool Wait(const std::vector<ref<Fence>>& fences, uint64_t TimeoutNanoseconds);
        EGX_API static void Reset(const std::vector<ref<Fence>>& fences);

        // Performs a wait on all fences for each frame
        EGX_API void SynchronizeAllFrames();

        EGX_API static ref<Fence> CreateSingleFence(const ref<VulkanCoreInterface>& CoreInterface, bool CreateSignaled = false);

        static ref<Fence> FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface, bool CreateSignaled = false) {
            return { new Fence(CoreInterface, CreateSignaled) };
        }
        
        Fence(Fence&) = delete;

    private:
        std::vector<VkFence> _fences;
        ref<VulkanCoreInterface> _core_interface = nullptr;
    };

}
