#include "CommandList.hpp"
#include "../core/backend/VkGraphicsCard.hpp"

/////=============================================== COMMAND POOL ===============================================/////
CommandPool CommandPool::Create(GraphicsContext context, bool ShortLivedListBuffers, bool AllowIndividualResets)
{
    PROFILE_FUNCTION();
    CommandPool pool;
    pool.m_ApiType = *(char *)context;
    pool.m_context = context;

    if (pool.m_ApiType == 0)
    {
        VkCommandPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        createInfo.flags = ((ShortLivedListBuffers) ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0) | ((AllowIndividualResets) ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0);
        auto vcont = ToVKContext(context);
        vkcheck(vkCreateCommandPool(vcont->defaultDevice, &createInfo, vcont->m_allocation_callback, &pool.m_pool));
    }

    return pool;
}

void CommandPool::Reset(bool ReleasePoolResources)
{
    if (m_ApiType == 0)
        vkcheck(vkResetCommandPool(GetVKDevice(m_context), m_pool, ReleasePoolResources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0));
}

void CommandPool::Destroy()
{
    PROFILE_FUNCTION();
    if (m_ApiType == 0)
    {
        auto vcont = ToVKContext(m_context);
        vkDestroyCommandPool(vcont->defaultDevice, m_pool, vcont->m_allocation_callback);
    }
}
