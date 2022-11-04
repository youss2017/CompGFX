#pragma once
#include "../core/egxcommon.hpp"
#include <vector>

namespace egx {

    class Semaphore {
    public:
        EGX_API Semaphore() = default;
        EGX_API Semaphore(const ref<VulkanCoreInterface>& CoreInterface, const std::string& Name);
        EGX_API Semaphore(Semaphore&&);
        EGX_API ~Semaphore();
        EGX_API void DelayInitalize(const ref<VulkanCoreInterface>& CoreInterface);
        EGX_API const VkSemaphore& GetSemaphore() const;

        EGX_API static ref<Semaphore> CreateSingleSemaphore(const ref<VulkanCoreInterface>& CoreInterface);
        EGX_API static std::vector<VkSemaphore> GetSemaphores(const std::vector<ref<Semaphore>>& semaphores, uint32_t CustomFrame = UINT32_MAX);
        EGX_API static std::vector<VkSemaphore>& GetSemaphores(const std::vector<ref<Semaphore>>& semaphores, std::vector<VkSemaphore>& OutSemaphores);

        static ref<Semaphore> FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface, const std::string& Name) {
            return new Semaphore(CoreInterface, Name);
        }

        EGX_API static const ref<Semaphore>& GetSemaphoreFromName(const std::string& Name, const std::vector<ref<Semaphore>>& Semaphores);

        Semaphore(Semaphore&) = delete;

    public:
        const std::string Name;

    private:
        friend class VulkanSwapchain;
        std::vector<VkSemaphore> _semaphores;
        ref<VulkanCoreInterface> _core_interface = nullptr;
        uint32_t* _currrent_frame_ptr = nullptr;
    };

}
