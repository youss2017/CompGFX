#include "semaphore.hpp"
#include <stdexcept>
#include <algorithm>
#include <Utility/CppUtility.hpp>
#include "../utils/objectdebugger.hpp"

namespace egx
{

    Semaphore::Semaphore(const ref<VulkanCoreInterface> &CoreInterface, const std::string &Name)
        : Name(Name)
    {
        DelayInitalize(CoreInterface);
        SetDebugName();
    }

    Semaphore::Semaphore(Semaphore &&move) noexcept
    {
        this->_semaphores = std::move(move._semaphores);
        this->_core_interface = move._core_interface;
    }

    Semaphore::~Semaphore()
    {
        if (_semaphores.size() > 0)
            for (size_t i = 0; i < _semaphores.size(); i++)
                vkDestroySemaphore(_core_interface->Device, _semaphores[i], nullptr);
    }

    void Semaphore::DelayInitalize(const ref<VulkanCoreInterface> &CoreInterface)
    {
        _core_interface = CoreInterface;
        _semaphores.resize(CoreInterface->MaxFramesInFlight);
        VkSemaphoreCreateInfo createInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        for (size_t i = 0; i < CoreInterface->MaxFramesInFlight; i++)
            vkCreateSemaphore(CoreInterface->Device, &createInfo, nullptr, &_semaphores[i]);
        DelayInitalizeFF(CoreInterface);
    }

    const VkSemaphore &Semaphore::GetSemaphore() const
    {
        return _semaphores[GetCurrentFrame()];
    }

    ref<Semaphore> Semaphore::CreateSingleSemaphore(const ref<VulkanCoreInterface> &CoreInterface)
    {
        ref<Semaphore> s = new Semaphore;
        s->_core_interface = CoreInterface;
        s->DelayInitalizeFF(CoreInterface);
        s->_semaphores.resize(1);
        VkSemaphoreCreateInfo createInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(CoreInterface->Device, &createInfo, nullptr, &s->_semaphores[0]);
        return s;
    }

    std::vector<VkSemaphore> Semaphore::GetSemaphores(const std::vector<ref<Semaphore>> &semaphores, uint32_t CustomFrame)
    {
        if(semaphores.size() == 0) return {};
        CustomFrame = CustomFrame == UINT32_MAX ? semaphores[0]->_core_interface->CurrentFrame : CustomFrame;
        std::vector<VkSemaphore> result;
        result.resize(semaphores.size());
        for (size_t i = 0; i < result.size(); i++)
            result[i] = semaphores[i]->_semaphores[CustomFrame];
        return result;
    }

    std::vector<VkSemaphore> &Semaphore::GetSemaphores(const std::vector<ref<Semaphore>> &semaphores, std::vector<VkSemaphore> &OutSemaphores)
    {
        if(semaphores.size() == 0) return OutSemaphores;
        OutSemaphores.clear();
        OutSemaphores.reserve(semaphores.size());
        for (size_t i = 0; i < semaphores.size(); i++)
            OutSemaphores.push_back(semaphores[i]->GetSemaphore());
        return OutSemaphores;
    }

    const ref<Semaphore> &Semaphore::GetSemaphoreFromName(const std::string &Name, const std::vector<ref<Semaphore>> &Semaphores)
    {
        const auto iterator = std::find_if(Semaphores.begin(), Semaphores.end(), [&](const ref<Semaphore> &x)
                                           { return x->Name == Name; });
        if (iterator == Semaphores.end())
            throw std::runtime_error("Could not find Semaphore from given name.");
        return *iterator;
    }

    void Semaphore::SetDebugName()
    {
        for (size_t i = 0; i < _semaphores.size(); i++)
        {
            SetObjectName(_core_interface, _semaphores[i], VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, cpp::Format("{0} [{1}]", Name, i));
        }
    }

}