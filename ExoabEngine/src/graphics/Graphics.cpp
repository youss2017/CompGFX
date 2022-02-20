#include "Graphics.hpp"
#include "CommandDefines.hpp"
#include "../utils/MBox.hpp"
#include "../utils/common.hpp"
#include "../utils/StringUtils.hpp"
#include "../utils/MBox.hpp"
#include "material_system/Material.hpp"
#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

// [NOTE]: ImGui multi-viewport uses VK_FORMAT_B8G8R8A8_UNORM, if we use a different format
// [NOTE]: there will be a mismatch of format between pipeline state objects and render pass
static constexpr VkFormat cs_SwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;

// =============================================== Graphics3D ===============================================
constexpr int OpenGL_MajorVersion = 3;
constexpr int OpenGL_MinorVersion = 3;

PFN_Graphics3D_WaitGPUIdle *Graphics3D_WaitGPUIdle = nullptr;
PFN_Graphics3D_SetSyncInterval *Graphics3D_SetSyncInterval = nullptr;
PFN_Graphics3D_Destroy *Graphics3D_Destroy = nullptr;

#if PROFILE_ENABLED == 2
void Graphics3D_Internal_DisplayProfileResults();
#endif;

IGraphics3D Graphics3D_Create(ConfigurationSettings *config, const char *Title, bool DebugMode, bool EnableImGui)
{
    Graphics3D *gfx = new Graphics3D();
    PlatformWindow *Window;
    Window = new PlatformWindow(Title, config->ResolutionWidth, config->ResolutionHeight);
    gfx->m_window = Window;
    gfx->m_EnableImGui = EnableImGui;
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

    VkPhysicalDeviceVulkan12Features vulkan12features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    vulkan12features.pNext = nullptr;;
    vulkan12features.drawIndirectCount = VK_TRUE;
    
    VkPhysicalDeviceShaderDrawParametersFeatures DrawParameters;
    DrawParameters.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    DrawParameters.pNext = &vulkan12features;
    DrawParameters.shaderDrawParameters = VK_TRUE;
    VkPhysicalDeviceSynchronization2FeaturesKHR Synchronization2Feature;
    Synchronization2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    Synchronization2Feature.pNext = &DrawParameters;
    Synchronization2Feature.synchronization2 = VK_FALSE;
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT DivisorFeature;
    DivisorFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT;
    DivisorFeature.pNext = &Synchronization2Feature;
    DivisorFeature.vertexAttributeInstanceRateDivisor = VK_TRUE;
    DivisorFeature.vertexAttributeInstanceRateZeroDivisor = VK_TRUE;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &DivisorFeature;
    features.features.fillModeNonSolid = VK_TRUE;
    features.features.samplerAnisotropy = VK_TRUE;
    features.features.sampleRateShading = VK_TRUE;
    features.features.multiDrawIndirect = VK_TRUE;
    std::vector<const char *> layer_extensions;
    layer_extensions.push_back("VK_KHR_get_physical_device_properties2");
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
    layers.push_back("VK_LAYER_KHRONOS_synchronization2");
    gfx->m_context = vk::Gfx_CreateContext(Window, DebugMode, config->ForceIntegeratedGPU, layers, layer_extensions,
                                               {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
                                                VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                                                VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                                                VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
                                                VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
                                                VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
                                                VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME},
                                               features, VK_API_VERSION_1_2);
    Graphics3D_LinkFunctions(gfx);
    auto vcont = ToVKContext(gfx->m_context);
    gfx->m_vswapchain = vk::GraphicsSwapchain::Create(vcont->instance, vcont->m_allocation_callback, vcont->card.handle, vcont->defaultDevice, vcont->defaultQueue, vcont->defaultQueueFamilyIndex, cs_SwapchainFormat, Window, config->FutureFrameCount, config->VSync, &vcont->FrameInfo, EnableImGui);
    config->FutureFrameCount = vcont->FrameInfo->m_FrameCount;
    gfx->m_FrameInfo = vcont->FrameInfo;
    vcont->pFrameIndex = &vcont->FrameInfo->m_FrameIndex;
  
    return gfx;
}

bool Graphics3D_PollEvents(IGraphics3D gfx)
{
    PROFILE_FUNCTION();
    gfx->m_window->Poll();
    return !gfx->m_window->ShouldClose();
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ VULKAN ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void Vulkan_Graphics3D_WaitGPUIdle(IGraphics3D gfx)
{
    PROFILE_FUNCTION();
    vkDeviceWaitIdle(ToVKContext(gfx->m_context)->defaultDevice);
}

void Vulkan_Graphics3D_SetSyncInterval(IGraphics3D gfx, int SyncInterval)
{
    // wait device idle and recreate swapchain with a different sync interval
    Graphics3D_WaitGPUIdle(gfx); // this is much easier than trying to synchronize and not wait idle.
    gfx->m_vswapchain.Destroy();
    auto vcont = ToVKContext(gfx->m_context);
    gfx->m_vswapchain = vk::GraphicsSwapchain::Create(vcont->instance, vcont->m_allocation_callback, vcont->card.handle, vcont->defaultDevice, vcont->defaultQueue, vcont->defaultQueueFamilyIndex, cs_SwapchainFormat, gfx->m_window, gfx->m_FrameInfo->m_FrameCount, SyncInterval, &vcont->FrameInfo, gfx->m_EnableImGui);
}

void Vulkan_Graphics3D_Destroy(IGraphics3D gfx)
{
    Graphics3D_WaitGPUIdle(gfx);
    gfx->m_vswapchain.Destroy();
    vk::Gfx_DestroyContext(ToVKContext(gfx->m_context));
    delete gfx->m_window;
    delete gfx;
}

bool Graphics3D_CheckVulkanSupport()
{
    return glfwVulkanSupported();
}

void Graphics3D_LinkFunctions(IGraphics3D gfx)
{
    if (gfx->m_ApiType == 0)
    {
        Graphics3D_WaitGPUIdle = Vulkan_Graphics3D_WaitGPUIdle;
        Graphics3D_SetSyncInterval = Vulkan_Graphics3D_SetSyncInterval;
        Graphics3D_Destroy = Vulkan_Graphics3D_Destroy;
    }
    else if (gfx->m_ApiType == 1)
    {
    }
    else
    {
        Utils::Break();
    }
    //GPUBuffer_LinkFunctions(gfx->m_context);
    GPUTexture2D_LinkFunctions(gfx->m_context);
    GPUTextureSampler_LinkFunctions(gfx->m_context);
    FramebufferStateManagment_LinkFunctions(gfx->m_context);
    Framebuffer_LinkFunctions(gfx->m_context);
    //PipelineLayout_LinkFunctions(gfx->m_context);
    PipelineState_LinkFunctions(gfx->m_context);
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
