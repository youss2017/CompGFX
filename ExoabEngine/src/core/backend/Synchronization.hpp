#pragma once
#include "backend_base.h"
#include <vulkan/vulkan.h>

struct GPUFence
{
    int m_ApiType;
    GraphicsContext m_context;
    VkFence m_fence;
};

typedef GPUFence* IGPUFence;

typedef IGPUFence PFN_GPUFence_Create(GraphicsContext context);
typedef bool PFN_GPUFence_IsFenceComplete(IGPUFence fence);
typedef bool PFN_GPUFence_WaitOnFences(uint32_t FenceCount, IGPUFence* pFences, uint32_t TimeOutNanoseconds);
typedef void PFN_GPUFence_ResetFences(uint32_t FenceCount, IGPUFence* pFences);
typedef void PFN_GPUFence_Destroy(IGPUFence fence);

extern PFN_GPUFence_Create* GPUFence_Create;
extern PFN_GPUFence_IsFenceComplete* GPUFence_IsFenceComplete;
extern PFN_GPUFence_WaitOnFences* GPUFence_WaitOnFences;
extern PFN_GPUFence_ResetFences* GPUFence_ResetFences;
extern PFN_GPUFence_Destroy* GPUFence_Destroy;

// Helper macro to get current frame fence for vulkan.
#define VkGPUFence_GetFence(fence) (fence->m_fence)

void GPUFence_LinkFunctions(GraphicsContext context);

struct GPUSemaphore
{
    int m_ApiType;
    GraphicsContext m_context;
    _FrameInformation* m_FrameInfo;
    VkSemaphore* m_semaphoreptr = NULL;
};

typedef GPUSemaphore* IGPUSemaphore;

typedef IGPUSemaphore PFN_GPUSemaphore_Create(GraphicsContext context, bool TimelineSemaphore);
typedef void PFN_GPUSemaphore_Destroy(IGPUSemaphore);

extern PFN_GPUSemaphore_Create* GPUSemaphore_Create;
extern PFN_GPUSemaphore_Destroy* GPUSemaphore_Destroy;

// Helper macro to get current frame semaphore for vulkan.
#define VkGPUSemaphore_GetSemaphore(semaphore) (semaphore->m_semaphoreptr[semaphore->m_FrameInfo->m_FrameIndex])

void GPUSemaphore_LinkFunctions(GraphicsContext context);