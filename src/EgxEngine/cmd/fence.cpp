#include "fence.hpp"
#include <Utility/CppUtility.hpp>

namespace egx
{
    using namespace ut;

    Fence::Fence(const ref<VulkanCoreInterface> &CoreInterface, bool CreateSignaled)
    {
        DelayInitalize(CoreInterface);
    }

    Fence::Fence(Fence &&move) noexcept
    {
        memcpy(this, &move, sizeof(Fence));
        memset(&move, 0, sizeof(Fence));
    }

    Fence::~Fence()
    {
        for(size_t i = 0; i < _fences.size(); i++)
            vkDestroyFence(_core_interface->Device, _fences[i], nullptr);
    }

    void Fence::DelayInitalize(const ref<VulkanCoreInterface> &CoreInterface, bool CreateSignaled)
    {
        _core_interface = CoreInterface;
        _current_frame_ptr = &CoreInterface->CurrentFrame;
        VkFenceCreateInfo createInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        createInfo.flags = CreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
        _fences.resize(CoreInterface->MaxFramesInFlight);
        for (size_t i = 0; i < _fences.size(); i++)
            vkCreateFence(_core_interface->Device, &createInfo, nullptr, &_fences[i]);
    }

    const VkFence& Fence::GetFence()
    {
        return _fences[GetCurrentFrame()];
    }

    bool Fence::Wait(uint64_t TimeoutNanoseconds)
    {
        return vkWaitForFences(_core_interface->Device, 1, &_fences[GetCurrentFrame()], VK_TRUE, TimeoutNanoseconds) != VK_SUCCESS;
    }

    void Fence::Reset()
    {
        vkResetFences(_core_interface->Device, 1, &_fences[GetCurrentFrame()]);
    }

    bool Fence::Wait(const std::vector<ref<Fence>> &fences, uint64_t TimeoutNanoseconds)
    {
        if (fences.size() == 0)
            return true;
        std::vector<VkFence> fence_handles(fences.size());
        for (size_t i = 0; i < fences.size(); i++)
            fence_handles[i] = fences[i]->GetFence();
        return vkWaitForFences(fences[0]->_core_interface->Device, (uint32_t)fence_handles.size(), fence_handles.data(), VK_TRUE, TimeoutNanoseconds) == VK_TRUE;
    }

    void Fence::Reset(const std::vector<ref<Fence>> &fences)
    {
        if (fences.size() == 0)
            return;
        std::vector<VkFence> fence_handles(fences.size());
        for (size_t i = 0; i < fences.size(); i++)
            fence_handles[i] = fences[i]->GetFence();
        vkResetFences(fences[0]->_core_interface->Device, (uint32_t)fence_handles.size(), fence_handles.data());
    }

    ref<Fence> Fence::CreateSingleFence(const ref<VulkanCoreInterface> &CoreInterface, bool CreateSignaled)
    {
        ref<Fence> f = {new Fence()};
        f->_core_interface = CoreInterface;
        VkFenceCreateInfo createInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        createInfo.flags = CreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
        f->_fences.resize(1);
        vkCreateFence(f->_core_interface->Device, &createInfo, nullptr, &f->_fences[0]);
        return f;
    }

}