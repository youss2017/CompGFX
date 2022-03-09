#include "Graphics.hpp"
#include "CommandDefines.hpp"
#include "../utils/MBox.hpp"
#include "../utils/common.hpp"
#include "../utils/StringUtils.hpp"
#include "../utils/MBox.hpp"
#include "material_system/Material.hpp"
#include <backend/GUI.h>
#include <GLFW/glfw3.h>

// [NOTE]: ImGui multi-viewport uses VK_FORMAT_B8G8R8A8_UNORM, if we use a different format
// [NOTE]: there will be a mismatch of format between pipeline state objects and render pass
static constexpr VkFormat cs_SwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;

// =============================================== Graphics3D ===============================================

#if PROFILE_ENABLED == 2
void Graphics3D_Internal_DisplayProfileResults();
#endif;

IGraphics3D Graphics3D_Create(ConfigurationSettings *config, const char *Title, bool DebugMode, bool EnableImGui)
{
    Graphics3D *gfx = new Graphics3D();
    PlatformWindow *Window;
    Window = new PlatformWindow(Title, config->ResolutionWidth, config->ResolutionHeight);
    gfx->m_window = Window;
    if (config->FutureFrameCount == 1)
    {
        logwarning("Future Frame Count must at least be 2");
        config->FutureFrameCount = 2;
    }
    if (config->ApiType == EngineAPIType::Vulkan && !glfwVulkanSupported())
    {
        config->ApiType = EngineAPIType::OpenGL;
        Utils::Message("API Alert", "Vulkan is not supported by current graphics card, therefore the Engine switched to OpenGL API.", Utils::WARNING);
    }
    gfx->m_ApiType = 0;

    /*
        Storage16 and Storage8 allow uint8_t, uint16_t, float16_t (and their derivatives) to be read but no arithmetics
        shaderInt8, shaderInt16, shaderFloat16 allow arithmetics but are not supported.
    */

    VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };
    DynamicRenderingFeature.pNext = nullptr;
    DynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkPhysicalDevice16BitStorageFeatures Storage16Bit{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES };
    Storage16Bit.pNext = &DynamicRenderingFeature;
    Storage16Bit.storageBuffer16BitAccess = VK_TRUE;
    Storage16Bit.uniformAndStorageBuffer16BitAccess = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vulkan12features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    vulkan12features.pNext = &Storage16Bit;
    vulkan12features.drawIndirectCount = VK_TRUE;
    vulkan12features.shaderInt8 = VK_FALSE;
    vulkan12features.storageBuffer8BitAccess = VK_TRUE;
    vulkan12features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
    vulkan12features.scalarBlockLayout = VK_TRUE;
    vulkan12features.descriptorIndexing = VK_TRUE;
    vulkan12features.runtimeDescriptorArray = VK_TRUE;
    vulkan12features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vulkan12features.shaderInt8 = VK_TRUE;
    vulkan12features.uniformBufferStandardLayout = VK_TRUE;
    vulkan12features.hostQueryReset = VK_TRUE;

    VkPhysicalDeviceShaderDrawParametersFeatures DrawParameters;
    DrawParameters.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    DrawParameters.pNext = &vulkan12features;
    DrawParameters.shaderDrawParameters = VK_TRUE;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &DrawParameters;
    features.features.fillModeNonSolid = VK_TRUE;
    features.features.samplerAnisotropy = VK_TRUE;
    features.features.sampleRateShading = VK_TRUE;
    features.features.multiDrawIndirect = VK_TRUE;
    std::vector<const char *> layer_extensions;
    uint32_t extensions_count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++) layer_extensions.push_back(extensions[i]);
    std::vector<const char *> layers;
    if (DebugMode)
    {
        layers.push_back("VK_LAYER_KHRONOS_validation");
        layer_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        layer_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
    bool ForceIntegeratedGPU;
#ifdef _DEBUG
    ForceIntegeratedGPU = true;
#else
    ForceIntegeratedGPU = false;
#endif
    gfx->m_context = vk::Gfx_CreateContext(Window, DebugMode, ForceIntegeratedGPU, layers, layer_extensions,
                                               {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
                                                VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                                                VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                                                VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
                                                VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                                                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
                                                },
                                               features, VK_API_VERSION_1_2);
    auto vcont = ToVKContext(gfx->m_context);
    gfx->m_vswapchain = vk::GraphicsSwapchain::Create(vcont->instance, vcont->m_allocation_callback, vcont->card.handle, vcont->defaultDevice, vcont->defaultQueue, vcont->defaultQueueFamilyIndex, cs_SwapchainFormat, Window, gFrameOverlapCount, config->VSync, EnableImGui);
    
    for (int i = 0; i < gFrameOverlapCount; i++)
    {
        gfx->m_FrameData[i].m_FrameIndex = i;
        gfx->m_FrameData[i].m_PresentSemaphore = vk::Gfx_CreateSemaphore(vcont, false);
        gfx->m_FrameData[i].m_RenderSemaphore = vk::Gfx_CreateSemaphore(vcont, false);
        gfx->m_FrameData[i].m_RenderFence = vk::Gfx_CreateFence(vcont, true);
        gfx->m_FrameData[i].m_pool = vk::Gfx_CreateCommandPool(vcont, false, true, false);
        gfx->m_FrameData[i].m_cmd = vk::Gfx_AllocCommandBuffer(vcont, gfx->m_FrameData[i].m_pool, true);
    }
    
    return gfx;
}

bool Graphics3D_PollEvents(IGraphics3D gfx)
{
    PROFILE_FUNCTION();
    gfx->m_window->Poll();
    return !gfx->m_window->ShouldClose();
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ VULKAN ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void Graphics3D_WaitGPUIdle(IGraphics3D gfx)
{
    PROFILE_FUNCTION();
    vkDeviceWaitIdle(ToVKContext(gfx->m_context)->defaultDevice);
}

void Graphics3D_SetSyncInterval(IGraphics3D gfx, int SyncInterval)
{
    // wait device idle and recreate swapchain with a different sync interval
    Graphics3D_WaitGPUIdle(gfx); // this is much easier than trying to synchronize and not wait idle.
    gfx->m_vswapchain.Destroy();
    auto vcont = ToVKContext(gfx->m_context);
    gfx->m_vswapchain = vk::GraphicsSwapchain::Create(vcont->instance, vcont->m_allocation_callback, vcont->card.handle, vcont->defaultDevice, vcont->defaultQueue, vcont->defaultQueueFamilyIndex, cs_SwapchainFormat, gfx->m_window, gFrameOverlapCount, SyncInterval, true);
}

void Graphics3D_Destroy(IGraphics3D gfx)
{
    Graphics3D_WaitGPUIdle(gfx);
    gfx->m_vswapchain.Destroy();
    for (int i = 0; i < gFrameOverlapCount; i++)
    {
        vkDestroySemaphore(ToVKContext(gfx->m_context)->defaultDevice, gfx->m_FrameData[i].m_PresentSemaphore, nullptr);
        vkDestroySemaphore(ToVKContext(gfx->m_context)->defaultDevice, gfx->m_FrameData[i].m_RenderSemaphore, nullptr);
        vkDestroyFence(ToVKContext(gfx->m_context)->defaultDevice, gfx->m_FrameData[i].m_RenderFence, nullptr);
        vkDestroyCommandPool(ToVKContext(gfx->m_context)->defaultDevice, gfx->m_FrameData[i].m_pool, nullptr);
    }
    vk::Gfx_DestroyContext(ToVKContext(gfx->m_context));
    delete gfx->m_window;
    delete gfx;
}

FrameData& Graphics3D_GetFrame(IGraphics3D gfx)
{
    return gfx->m_FrameData[gfx->m_FrameCount % gFrameOverlapCount];
}

bool Graphics3D_CheckVulkanSupport()
{
    return glfwVulkanSupported() == GLFW_TRUE;
}

#if PROFILE_ENABLED == 2
void Graphics3D_Internal_DisplayProfileResults()
{
    ImGui::Begin("Profile Results");

    for (const auto &result : PROFILE_RESULTS)
    {
        char label[50];
        strcpy(label, "%0.3fms ");
        strcat(label, result.m_Name);
        ImGui::Text(label, result.m_Duration);
    }

    ImGui::End();
    PROFILE_RESULTS.clear();
}
#endif
