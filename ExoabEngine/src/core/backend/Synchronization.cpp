#include "Synchronization.hpp"
#include "VkGraphicsCard.hpp"

//=============================================== GPU FENCE ===============================================
IGPUFence Vulkan_GPUFence_Create(GraphicsContext context)
{
    IGPUFence fence = new GPUFence();
    fence->m_ApiType = 1;
    fence->m_context = context;
    auto vcont = ToVKContext(context);
    assert(vcont->FrameInfo->m_Initalized);
    fence->m_fence = vk::Gfx_CreateFence(vcont);

    return fence;
}

bool Vulkan_GPUFence_IsFenceComplete(IGPUFence fence)
{
    auto vcont = ToVKContext(fence->m_context);
    return vkWaitForFences(vcont->defaultDevice, 1, &fence->m_fence, true, 0) == VK_SUCCESS;
}

bool Vulkan_GPUFence_WaitOnFences(uint32_t FenceCount, IGPUFence* pFences, uint32_t TimeOutNanoseconds)
{
    auto context = ToVKContext(pFences[0]->m_context);
    bool FenceCompleted = false;
    if (FenceCount == 0)
        return true;
    else if (FenceCount == 1)
    {
        FenceCompleted = vkWaitForFences(context->defaultDevice, 1, &pFences[0]->m_fence, true, TimeOutNanoseconds) == VK_SUCCESS;
    }
    else
    {
        VkFence* vfences = (VkFence*)alloca(sizeof(VkFence) * FenceCount);
        for (uint32_t i = 0; i < FenceCount; i++)
        {
            vfences[i] = pFences[i]->m_fence;
        }
        FenceCompleted = vkWaitForFences(context->defaultDevice, FenceCount, vfences, true, TimeOutNanoseconds) == VK_SUCCESS;
    }
    return FenceCompleted;
}

void Vulkan_GPUFence_ResetFences(uint32_t FenceCount, IGPUFence* pFences)
{
    auto vcont = ToVKContext(pFences[0]->m_context);
if (FenceCount == 0)
        return;
    else if (FenceCount == 1)
    {
        vkResetFences(vcont->defaultDevice, FenceCount, &pFences[0]->m_fence);
    }
    else {
        VkFence* pF = (VkFence*)alloca(sizeof(VkFence) * FenceCount);
        for (uint32_t i = 0; i < FenceCount; i++)
        {
            pF[i] = pFences[i]->m_fence;
        }
        vkResetFences(vcont->defaultDevice, FenceCount, pF);
    }
}

void Vulkan_GPUFence_Destroy(IGPUFence fence)
{
    auto vcont = ToVKContext(fence->m_context);
    vkDestroyFence(vcont->defaultDevice, fence->m_fence, vcont->m_allocation_callback);
    delete fence;
}

IGPUFence GL_GPUFence_Create(GraphicsContext context) { return nullptr; }
bool GL_GPUFence_IsFenceComplete(IGPUFence fence) { return true; }
bool GL_GPUFence_WaitOnFences(uint32_t FenceCount, IGPUFence* pFences, uint32_t TimeOutNanoseconds) { return true; }
void GL_GPUFence_ResetFences(uint32_t FenceCount, IGPUFence* pFences) {}
void GL_GPUFence_Destroy(IGPUFence fence) {}

PFN_GPUFence_Create* GPUFence_Create = nullptr;
PFN_GPUFence_IsFenceComplete* GPUFence_IsFenceComplete = nullptr;
PFN_GPUFence_WaitOnFences* GPUFence_WaitOnFences = nullptr;
PFN_GPUFence_ResetFences* GPUFence_ResetFences = nullptr;
PFN_GPUFence_Destroy* GPUFence_Destroy = nullptr;

void GPUFence_LinkFunctions(GraphicsContext context)
{
    char at = *(char*)context;
    if (at == 0)
    {
        GPUFence_Create = Vulkan_GPUFence_Create;
        GPUFence_IsFenceComplete = Vulkan_GPUFence_IsFenceComplete;
        GPUFence_WaitOnFences = Vulkan_GPUFence_WaitOnFences;
        GPUFence_ResetFences = Vulkan_GPUFence_ResetFences;
        GPUFence_Destroy = Vulkan_GPUFence_Destroy;
    }
    else if (at == 1)
    {
        GPUFence_Create = GL_GPUFence_Create;
        GPUFence_IsFenceComplete = GL_GPUFence_IsFenceComplete;
        GPUFence_WaitOnFences = GL_GPUFence_WaitOnFences;
        GPUFence_ResetFences = GL_GPUFence_ResetFences;
        GPUFence_Destroy = GL_GPUFence_Destroy;
    }
    else
        assert(0);
}

// =============================================== GPU SEMAPHORE ===============================================

PFN_GPUSemaphore_Create* GPUSemaphore_Create = nullptr;
PFN_GPUSemaphore_Destroy* GPUSemaphore_Destroy = nullptr;

IGPUSemaphore Vulkan_GPUSemaphore_Create(GraphicsContext context, bool TimelineSemaphore)
{
    IGPUSemaphore semaphore = new GPUSemaphore();
    semaphore->m_ApiType = *(char*)context;
    semaphore->m_context = context;
    auto vcont = ToVKContext(context);
    assert(vcont->FrameInfo->m_Initalized);
    semaphore->m_semaphoreptr = new VkSemaphore[vcont->FrameInfo->m_FrameCount];
    for (uint32_t i = 0; i < vcont->FrameInfo->m_FrameCount; i++)
        semaphore->m_semaphoreptr[i] = vk::Gfx_CreateSemaphore(vcont, TimelineSemaphore);
    semaphore->m_FrameInfo = vcont->FrameInfo;
    return semaphore;
}

void Vulkan_GPUSemaphore_Destroy(IGPUSemaphore semaphore)
{
    auto vcont = ToVKContext(semaphore->m_context);
    for (uint32_t i = 0; i < semaphore->m_FrameInfo->m_FrameCount; i++)
        vkDestroySemaphore(vcont->defaultDevice, semaphore->m_semaphoreptr[i], vcont->m_allocation_callback);
    delete[] semaphore->m_semaphoreptr;
}

IGPUSemaphore GL_GPUSemaphore_Create(GraphicsContext context, bool TimelineSemaphore) { return nullptr; }
void GL_GPUSemaphore_Destroy(IGPUSemaphore) {}

void GPUSemaphore_LinkFunctions(GraphicsContext context)
{
    char at = *(char*)context;
    if (at == 0)
    {
        GPUSemaphore_Create = Vulkan_GPUSemaphore_Create;
        GPUSemaphore_Destroy = Vulkan_GPUSemaphore_Destroy;
    }
    else if (at == 1)
    {
        GPUSemaphore_Create = GL_GPUSemaphore_Create;
        GPUSemaphore_Destroy = GL_GPUSemaphore_Destroy;
    }
    else
        assert(0);
}
