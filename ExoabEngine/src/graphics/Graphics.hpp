#pragma once
#include "../core/backend/backend_base.h"
#include "../window/PlatformWindow.hpp"
#include "../core/backend/VkGraphicsCard.hpp"
#include "../core/backend/Swapchain.hpp"
#include "../core/pipeline/Framebuffer.hpp"
#include "../utils/Profiling.hpp"
#include "../core/pipeline/Pipeline.hpp"
#include <vector>

struct ConfigurationSettings
{
    std::string WindowTitle = "UNDEFINED";
    EngineAPIType ApiType = EngineAPIType::Vulkan;
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
    FrameData m_FrameData[gFrameOverlapCount];
    GraphicsContext m_context;
    PlatformWindow *m_window;
    vk::GraphicsSwapchain m_vswapchain;
    uint64_t m_FrameCount = 0;
};

typedef Graphics3D *IGraphics3D;

void Graphics3D_WaitGPUIdle(IGraphics3D gfx);
void Graphics3D_SetSyncInterval(IGraphics3D gfx, int SyncInterval);
void Graphics3D_Destroy(IGraphics3D gfx);

FrameData& Graphics3D_GetFrame(IGraphics3D gfx);

IGraphics3D Graphics3D_Create(ConfigurationSettings* config, const char *Title, bool DebugMode, bool EnableImGui, bool RenderDOC);
bool Graphics3D_CheckVulkanSupport();
bool Graphics3D_PollEvents(IGraphics3D gfx);

static inline void Graphics3D_NextFrame(IGraphics3D gfx) { gfx->m_FrameCount++; }
