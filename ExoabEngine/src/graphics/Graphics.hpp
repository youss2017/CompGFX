#pragma once
#include "../core/backend/backend_base.h"
#include "../core/backend/Synchronization.hpp"
#include "../core/memory/Textures.hpp"
#include "../window/PlatformWindow.hpp"
#include "../core/backend/GLGraphicsCard.hpp"
#include "../core/backend/VkGraphicsCard.hpp"
#include "../core/backend/Swapchain.hpp"
#include "../core/backend/GLSwapchain.hpp"
#include "../core/pipeline/Framebuffer.hpp"
#include "../utils/Profiling.hpp"
#include "CommandList.hpp"
#include <vector>
#include <mutex>
#include <atomic>

struct ConfigurationSettings
{
    EngineAPIType ApiType = EngineAPIType::OpenGL;
    uint32_t ResolutionWidth = 800;
    uint32_t ResolutionHeight = 600;
    bool Fullscreen = false;
    TextureSamples MSAASamples = TextureSamples::MSAA_1;
    bool VSync = true;
    uint32_t FutureFrameCount = 3;
    bool ForceIntegeratedGPU = false;
};

struct Graphics3D
{
    int m_ApiType;
    _FrameInformation *m_FrameInfo;
    GraphicsContext m_context;
    PlatformWindow *m_window;
    vk::GraphicsSwapchain m_vswapchain;
    gl::GraphicsSwapchain m_gswapchain;
    bool m_EnableImGui;
    // To reduce driver calls to OpenGL (when settings the pipeline)
    std::atomic<PipelineSpecification> m_PipelineSpecification;
    bool m_PipelineInitalized = false; // OpenGL pipeline flag
    std::mutex m_OpenGLLock;
};

typedef Graphics3D *IGraphics3D;

typedef void PFN_Graphics3D_WaitGPUIdle(IGraphics3D gfx);
typedef void PFN_Graphics3D_ExecuteCommandList(IGraphics3D gfx, ICommandList CmdList, IGPUFence WaitFence, uint32_t WaitSemaphoreCount, IGPUSemaphore *pWaitSemaphores, uint32_t SignalSemaphoreCount, IGPUSemaphore *pSignalSemaphores);
typedef void PFN_Graphics3D_ExecuteCommandLists(IGraphics3D gfx, uint32_t CmdListCount, ICommandList *pCmdLists, IGPUFence WaitFence, uint32_t WaitSemaphoreCount, IGPUSemaphore *pWaitSemaphores, uint32_t SignalSemaphoreCount, IGPUSemaphore *pSignalSemaphores);
typedef void PFN_Graphics3D_SetSyncInterval(IGraphics3D gfx, int SyncInterval);
typedef void PFN_Graphics3D_PrepareNextFrame(IGraphics3D gfx, IGPUSemaphore *pSwapchainSemaphoreReady);
typedef void PFN_Graphics3D_Present(IGraphics3D gfx, IFramebuffer framebuffer, uint32_t AttachmentIndex, uint32_t WaitSemaphoreCount, IGPUSemaphore* pWaitSemahpores);
typedef void PFN_Graphics3D_Destroy(IGraphics3D gfx);

extern PFN_Graphics3D_WaitGPUIdle* Graphics3D_WaitGPUIdle;
extern PFN_Graphics3D_ExecuteCommandList* Graphics3D_ExecuteCommandList;
extern PFN_Graphics3D_ExecuteCommandLists* Graphics3D_ExecuteCommandLists;
extern PFN_Graphics3D_SetSyncInterval* Graphics3D_SetSyncInterval;
extern PFN_Graphics3D_PrepareNextFrame* Graphics3D_PrepareNextFrame;
extern PFN_Graphics3D_Present* Graphics3D_Present;
extern PFN_Graphics3D_Destroy* Graphics3D_Destroy;

void Graphics3D_LinkFunctions(IGraphics3D gfx);
IGraphics3D Graphics3D_Create(ConfigurationSettings* config, const char *Title, bool DebugMode, bool EnableImGui);
bool Graphics3D_CheckVulkanSupport();
bool Graphics3D_PollEvents(IGraphics3D gfx);
