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
PFN_Graphics3D_ExecuteCommandList *Graphics3D_ExecuteCommandList = nullptr;
PFN_Graphics3D_ExecuteCommandLists *Graphics3D_ExecuteCommandLists = nullptr;
PFN_Graphics3D_SetSyncInterval *Graphics3D_SetSyncInterval = nullptr;
PFN_Graphics3D_PrepareNextFrame *Graphics3D_PrepareNextFrame = nullptr;
PFN_Graphics3D_Present *Graphics3D_Present = nullptr;
PFN_Graphics3D_Destroy *Graphics3D_Destroy = nullptr;

static bool GL_Graphics3D_Internal_CheckOpenGLSupport(gl::GLContext context);
#if PROFILE_ENABLED == 2
void Graphics3D_Internal_DisplayProfileResults();
#endif;

IGraphics3D Graphics3D_Create(ConfigurationSettings *config, const char *Title, bool DebugMode, bool EnableImGui)
{
    Graphics3D *gfx = new Graphics3D();
    PlatformWindow *Window;
    if (config->ApiType == EngineAPIType::Vulkan)
        Window = new PlatformWindow(Title, config->ResolutionWidth, config->ResolutionHeight, true);
    else
        Window = new PlatformWindow(Title, config->ResolutionWidth, config->ResolutionHeight, false);
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
    if (config->ApiType == EngineAPIType::Vulkan)
    {
        gfx->m_ApiType = 0;
        VkPhysicalDeviceShaderDrawParametersFeatures DrawParameters;
        DrawParameters.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
        DrawParameters.pNext = nullptr;
        DrawParameters.shaderDrawParameters = VK_TRUE;
        VkPhysicalDeviceTimelineSemaphoreFeatures TimelineSemaphoreFeature;
        TimelineSemaphoreFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        TimelineSemaphoreFeature.pNext = &DrawParameters;
        TimelineSemaphoreFeature.timelineSemaphore = VK_TRUE;
        VkPhysicalDeviceSynchronization2FeaturesKHR Synchronization2Feature;
        Synchronization2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
        Synchronization2Feature.pNext = &TimelineSemaphoreFeature;
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
    }
    else
    {
        gfx->m_ApiType = 1;
        gfx->m_context = gl::Gfx_CreateContext(Window, OpenGL_MajorVersion, OpenGL_MinorVersion, DebugMode);
        Graphics3D_LinkFunctions(gfx);
        bool GLSupport = GL_Graphics3D_Internal_CheckOpenGLSupport((gl::GLContext)gfx->m_context);
        if (!GLSupport)
        {
            logerror("Cannot run this application since some OpenGL features are not supported!");
            exit(0x09e21);
        }
        else
            loginfo("All Required OpenGL features are Supported.");
        //((gl::GLContext)gfx->m_context)->SupportUniformBufferObject = false;
        gfx->m_gswapchain = gl::GraphicsSwapchain::Create(ToGLContext(gfx->m_context), Window, EnableImGui);
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

void Vulkan_Graphics3D_WaitGPUIdle(IGraphics3D gfx)
{
    PROFILE_FUNCTION();
    vkDeviceWaitIdle(ToVKContext(gfx->m_context)->defaultDevice);
}

void Vulkan_Graphics3D_ExecuteCommandList(IGraphics3D gfx, ICommandList CmdList, IGPUFence WaitFence, uint32_t WaitSemaphoreCount, IGPUSemaphore *pWaitSemaphores, uint32_t SignalSemaphoreCount, IGPUSemaphore *pSignalSemaphores)
{
    PROFILE_FUNCTION();
    // stack memory allocation
    VkSemaphore *_pWaitSemaphores = (VkSemaphore *)alloca(sizeof(VkSemaphore) * WaitSemaphoreCount);
    VkSemaphore *_pSignalSemaphores = (VkSemaphore *)alloca(sizeof(VkSemaphore) * SignalSemaphoreCount);
    VkPipelineStageFlags *_pWaitDstStageMask = (VkPipelineStageFlags *)alloca(sizeof(VkPipelineStageFlags) * WaitSemaphoreCount);
    for (uint32_t i = 0; i < WaitSemaphoreCount; i++)
    {
        _pWaitSemaphores[i] = VkGPUSemaphore_GetSemaphore(pWaitSemaphores[i]);
        _pWaitDstStageMask[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO: Make this flexiable
    }
    for (uint32_t i = 0; i < SignalSemaphoreCount; i++)
    {
        _pSignalSemaphores[i] = VkGPUSemaphore_GetSemaphore(pSignalSemaphores[i]);
    }
    VkSubmitInfo SubmitInfo;
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = nullptr;
    SubmitInfo.waitSemaphoreCount = WaitSemaphoreCount;
    SubmitInfo.pWaitSemaphores = _pWaitSemaphores;
    SubmitInfo.pWaitDstStageMask = _pWaitDstStageMask;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &VkCommandList_Get(CmdList);//&CmdList->m_Cmd;
    SubmitInfo.signalSemaphoreCount = SignalSemaphoreCount;
    SubmitInfo.pSignalSemaphores = _pSignalSemaphores;
    VkFence fence = (WaitFence) ? VkGPUFence_GetFence(WaitFence) : nullptr;
    vkcheck(vkQueueSubmit(ToVKContext(gfx->m_context)->defaultQueue, 1, &SubmitInfo, fence));
}

void Vulkan_Graphics3D_ExecuteCommandLists(IGraphics3D gfx, uint32_t CmdListCount, ICommandList *pCmdLists, IGPUFence WaitFence, uint32_t WaitSemaphoreCount, IGPUSemaphore *pWaitSemaphores, uint32_t SignalSemaphoreCount, IGPUSemaphore *pSignalSemaphores)
{
    PROFILE_FUNCTION();
    // stack memory allocation
    VkSemaphore *_pWaitSemaphores = (VkSemaphore *)alloca(sizeof(VkSemaphore) * WaitSemaphoreCount);
    VkSemaphore *_pSignalSemaphores = (VkSemaphore *)alloca(sizeof(VkSemaphore) * SignalSemaphoreCount);
    VkPipelineStageFlags *_pWaitDstStageMask = (VkPipelineStageFlags *)alloca(sizeof(VkPipelineStageFlags) * WaitSemaphoreCount);
    VkCommandBuffer *_pCmdLists = (VkCommandBuffer *)alloca(sizeof(VkCommandBuffer) * CmdListCount);
    for (uint32_t i = 0; i < WaitSemaphoreCount; i++)
    {
        _pWaitSemaphores[i] = VkGPUSemaphore_GetSemaphore(pWaitSemaphores[i]);
        _pWaitDstStageMask[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO: Make this flexiable
    }
    for (uint32_t i = 0; i < SignalSemaphoreCount; i++)
    {
        _pSignalSemaphores[i] = VkGPUSemaphore_GetSemaphore(pSignalSemaphores[i]);
    }
    for (uint32_t i = 0; i < CmdListCount; i++)
    {
        _pCmdLists[i] = VkCommandList_Get(pCmdLists[i]);//(VkCommandBuffer)pCmdLists[i].m_Cmd;
    }
    VkSubmitInfo SubmitInfo;
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = nullptr;
    SubmitInfo.waitSemaphoreCount = WaitSemaphoreCount;
    SubmitInfo.pWaitSemaphores = _pWaitSemaphores;
    SubmitInfo.pWaitDstStageMask = _pWaitDstStageMask;
    SubmitInfo.commandBufferCount = CmdListCount;
    SubmitInfo.pCommandBuffers = _pCmdLists;
    SubmitInfo.signalSemaphoreCount = SignalSemaphoreCount;
    SubmitInfo.pSignalSemaphores = _pSignalSemaphores;
    vkcheck(vkQueueSubmit(ToVKContext(gfx->m_context)->defaultQueue, 1, &SubmitInfo, VkGPUFence_GetFence(WaitFence)));
}
void Vulkan_Graphics3D_SetSyncInterval(IGraphics3D gfx, int SyncInterval)
{
    // wait device idle and recreate swapchain with a different sync interval
    Graphics3D_WaitGPUIdle(gfx); // this is much easier than trying to synchronize and not wait idle.
    gfx->m_vswapchain.Destroy();
    auto vcont = ToVKContext(gfx->m_context);
    gfx->m_vswapchain = vk::GraphicsSwapchain::Create(vcont->instance, vcont->m_allocation_callback, vcont->card.handle, vcont->defaultDevice, vcont->defaultQueue, vcont->defaultQueueFamilyIndex, cs_SwapchainFormat, gfx->m_window, gfx->m_FrameInfo->m_FrameCount, SyncInterval, &vcont->FrameInfo, gfx->m_EnableImGui);
}

void Vulkan_Graphics3D_PrepareNextFrame(IGraphics3D gfx, IGPUSemaphore *pSwapchainSemaphoreReady)
{
    PROFILE_FUNCTION();
    assert(pSwapchainSemaphoreReady);
    gfx->m_FrameInfo->m_LastFrameIndex = gfx->m_FrameInfo->m_FrameIndex;
    gfx->m_vswapchain.PrepareNextFrame(&gfx->m_FrameInfo->m_FrameIndex, pSwapchainSemaphoreReady);
}

void Vulkan_Graphics3D_Present(IGraphics3D gfx, IFramebuffer framebuffer, uint32_t AttachmentIndex, uint32_t WaitSemaphoreCount, IGPUSemaphore* pWaitSemahpores, bool DepthPipeline)
{
#if PROFILE_ENABLED == 2
    if (gfx->m_EnableImGui)
    {
        Graphics3D_Internal_DisplayProfileResults();
    }
#endif
    VkSemaphore wait_semaphores[50];
    for (uint32_t i = 0; i < WaitSemaphoreCount; i++)
        wait_semaphores[i] = VkGPUSemaphore_GetSemaphore(pWaitSemahpores[i]);
    if (gfx->m_window->m_width && gfx->m_window->m_height)
        gfx->m_vswapchain.Present((VkImage)Framebuffer_GetImage(framebuffer, AttachmentIndex), (VkImageView)Framebuffer_GetView(framebuffer, AttachmentIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, WaitSemaphoreCount, wait_semaphores, DepthPipeline);
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

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ OPENGL ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#define CheckFunction(x, str, flag)                              \
    if (!x)                                                      \
    {                                                            \
        str += "Missing Function --> " + std::string(#x) + "\n"; \
        flag = false;                                            \
    }
#define CheckExtension(x, str, flag)                              \
    if (!gl::Gfx_CheckExtensionSupport(x))                        \
    {                                                             \
        str += "Missing Extension ---> " + std::string(x) + "\n"; \
        flag = false;                                             \
    }
static bool GL_Graphics3D_Internal_CheckOpenGLSupport(gl::GLContext context)
{
    std::string missing_functions = "These are required functions for the engine to function using OpenGL\n";
    std::string missing_preferred_functions = "The following are preferred functions (typically for best performance)\n";
    std::string missing_extensions = "These are required extensions for the engine to function using OpenGL\n";
    std::string missing_essential_extensions = "The following are preferred extensions (typically for best performance)\n";
    bool flag = true;
    bool flag_preferred = true;
    bool flag_extensions = true;
    bool flag_essential_extensions = true;
    CheckFunction(glCreateShader, missing_functions, flag);
    CheckFunction(glShaderSource, missing_functions, flag);
    CheckFunction(glCompileShader, missing_functions, flag);
    CheckFunction(glGetShaderiv, missing_functions, flag);
    CheckFunction(glGetShaderInfoLog, missing_functions, flag);
    CheckFunction(glAttachShader, missing_functions, flag);
    CheckFunction(glLinkProgram, missing_functions, flag);
    CheckFunction(glGetProgramInfoLog, missing_functions, flag);
    CheckFunction(glDeleteShader, missing_functions, flag);
    CheckFunction(glGenVertexArrays, missing_functions, flag);
    CheckFunction(glGetUniformLocation, missing_functions, flag);
    CheckFunction(glUseProgram, missing_functions, flag);
    CheckFunction(glBindVertexArray, missing_functions, flag);
    CheckFunction(glUniform1i, missing_functions, flag);
    CheckFunction(glBindFramebuffer, missing_functions, flag);
    CheckFunction(glViewport, missing_functions, flag);
    CheckFunction(glScissor, missing_functions, flag);
    CheckFunction(glActiveTexture, missing_functions, flag);
    CheckFunction(glBindTexture, missing_functions, flag);
    CheckFunction(glDrawArrays, missing_functions, flag);
    CheckFunction(glDrawElements, missing_functions, flag);
    CheckFunction(glDeleteVertexArrays, missing_functions, flag);
    CheckFunction(glDeleteProgram, missing_functions, flag);
    CheckFunction(glGenBuffers, missing_functions, flag);
    CheckFunction(glBindBuffer, missing_functions, flag);
    CheckFunction(glBufferData, missing_functions, flag);
    CheckFunction(glMapBuffer, missing_functions, flag);
    CheckFunction(glUnmapBuffer, missing_functions, flag);
    CheckFunction(glDeleteBuffers, missing_functions, flag);
    CheckFunction(glGenTextures, missing_functions, flag);
    CheckFunction(glTexParameteri, missing_functions, flag);
    CheckFunction(glTexParameterf, missing_functions, flag);
    CheckFunction(glGetIntegerv, missing_functions, flag);
    CheckFunction(glTexImage2D, missing_functions, flag);
    CheckFunction(glGenerateMipmap, missing_functions, flag);
    CheckFunction(glDeleteTextures, missing_functions, flag);
    CheckFunction(glGenSamplers, missing_functions, flag);
    CheckFunction(glSamplerParameteri, missing_functions, flag);
    CheckFunction(glDeleteSamplers, missing_functions, flag);
    CheckFunction(glGenFramebuffers, missing_functions, flag);
    CheckFunction(glBindFramebuffer, missing_functions, flag);
    CheckFunction(glFramebufferTexture2D, missing_functions, flag);
    CheckFunction(glCheckFramebufferStatus, missing_functions, flag);
    CheckFunction(glEnable, missing_functions, flag);
    CheckFunction(glDisable, missing_functions, flag);
    CheckFunction(glBufferSubData, missing_functions, flag);
    CheckFunction(glFinish, missing_functions, flag);
    CheckFunction(glCullFace, missing_functions, flag);
    CheckFunction(glDepthRange, missing_functions, flag);
    CheckFunction(glDepthFunc, missing_functions, flag);
    CheckFunction(glDepthMask, missing_functions, flag);
    CheckFunction(glFrontFace, missing_functions, flag);
    CheckFunction(glPolygonMode, missing_functions, flag);
    CheckFunction(glDrawElementsInstancedBaseVertex, missing_functions, flag);
    CheckFunction(glDrawArraysInstanced, missing_functions, flag);
    CheckFunction(glDrawBuffer, missing_functions, flag);
    CheckFunction(glDrawBuffers, missing_functions, flag);
    CheckFunction(glColorMaski, missing_functions, flag);
    CheckFunction(glEnableVertexAttribArray, missing_functions, flag);
    CheckFunction(glVertexAttribPointer, missing_functions, flag);
    CheckFunction(glVertexAttribIPointer, missing_functions, flag);
    CheckFunction(glUniformMatrix4fv, missing_functions, flag);
    CheckFunction(glUniformMatrix3fv, missing_functions, flag);
    CheckFunction(glUniformMatrix2fv, missing_functions, flag);
    CheckFunction(glUniform1f, missing_functions, flag);
    CheckFunction(glUniform1i, missing_functions, flag);
    CheckFunction(glUniform1ui, missing_functions, flag);
    CheckFunction(glVertexAttribDivisor, missing_functions, flag);

    CheckFunction(glBufferStorage, missing_preferred_functions, flag_preferred);
    CheckFunction(glTextureStorage1D, missing_preferred_functions, flag_preferred);
    CheckFunction(glTextureStorage2D, missing_preferred_functions, flag_preferred);
    CheckFunction(glTextureStorage3D, missing_preferred_functions, flag_preferred);
    CheckFunction(glMinSampleShading, missing_preferred_functions, flag_preferred);

    CheckExtension("GL_ARB_debug_output", missing_extensions, flag_extensions);
    CheckExtension("GL_ARB_uniform_buffer_object", missing_extensions, flag_extensions);
    CheckExtension("GL_ARB_shading_language_420pack", missing_extensions, flag_extensions);

    // Set supported features
    if (glBufferStorage)
        context->SupportBufferStorage = true;
    if (glTextureStorage1D && glTextureStorage2D && glTextureStorage3D)
        context->SupportTextureStorage = true;
    if (glMinSampleShading)
        context->SupportMinSampleShading = true;
    if (gl::Gfx_CheckExtensionSupport("GL_ARB_uniform_buffer_object"))
        context->SupportUniformBufferObject = true;
    if (gl::Gfx_CheckExtensionSupport("GL_ARB_shading_language_420pack"))
        context->SupportShadingLanguage420Pack = true;

    if (!flag)
        logerrors(missing_functions);
    if (!flag_preferred)
        logwarnings(missing_preferred_functions); // Utils::Message("Missing OpenGL Features", missing_preferred_functions.c_str(), Utils::MBox::WARNING);
    if (!flag_extensions)
        logwarnings(missing_extensions); // Utils::Message("Missing OpenGL Features", missing_extensions.c_str(), Utils::MBox::WARNING);
    if (!flag_essential_extensions)
        logerrors(missing_essential_extensions);

    return flag && flag_essential_extensions;
}
#undef CheckFunction
#undef CheckExtension

void GL_Graphics3D_WaitGPUIdle(IGraphics3D gfx)
{
    PROFILE_FUNCTION();
    glFinish();
}

void GL_Graphics3D_ExecuteCommandList(IGraphics3D gfx, ICommandList CmdList, IGPUFence WaitFence, uint32_t WaitSemaphoreCount, IGPUSemaphore *pWaitSemaphores, uint32_t SignalSemaphoreCount, IGPUSemaphore *pSignalSemaphores);

void GL_Graphics3D_ExecuteCommandLists(IGraphics3D gfx, uint32_t CmdListCount, ICommandList *pCmdLists, IGPUFence WaitFence, uint32_t WaitSemaphoreCount, IGPUSemaphore *pWaitSemaphores, uint32_t SignalSemaphoreCount, IGPUSemaphore *pSignalSemaphores)
{
    PROFILE_FUNCTION();
    for (uint32_t i = 0; i < CmdListCount; i++)
    {
        GL_Graphics3D_ExecuteCommandList(gfx, pCmdLists[i], WaitFence, 0, nullptr, 0, nullptr);
    }
}

void GL_Graphics3D_SetSyncInterval(IGraphics3D gfx, int SyncInterval)
{
    gl::Gfx_SetVSyncState(SyncInterval);
}
void GL_Graphics3D_PrepareNextFrame(IGraphics3D gfx, IGPUSemaphore *pSwapchainSemaphoreReady)
{
    PROFILE_FUNCTION();
    gfx->m_gswapchain.PrepareNextFrame();
}

void GL_Graphics3D_Present(IGraphics3D gfx, IFramebuffer framebuffer, uint32_t AttachmentIndex, uint32_t WaitSemaphoreCount, IGPUSemaphore* pWaitSemahpores, bool DepthPipeline)
{
    PROFILE_FUNCTION();
#if PROFILE_ENABLED == 2
    if (gfx->m_EnableImGui)
    {
        Graphics3D_Internal_DisplayProfileResults();
    }
#endif
    gfx->m_gswapchain.Present(framebuffer, AttachmentIndex);
}

void GL_Graphics3D_Destroy(IGraphics3D gfx)
{
    gfx->m_gswapchain.Destroy();
    gl::Gfx_DestroyContext(ToGLContext(gfx->m_context));
    delete gfx->m_window;
    delete gfx;
}

void Graphics3D_LinkFunctions(IGraphics3D gfx)
{
    if (gfx->m_ApiType == 0)
    {
        Graphics3D_WaitGPUIdle = Vulkan_Graphics3D_WaitGPUIdle;
        Graphics3D_ExecuteCommandList = Vulkan_Graphics3D_ExecuteCommandList;
        Graphics3D_ExecuteCommandLists = Vulkan_Graphics3D_ExecuteCommandLists;
        Graphics3D_SetSyncInterval = Vulkan_Graphics3D_SetSyncInterval;
        Graphics3D_PrepareNextFrame = Vulkan_Graphics3D_PrepareNextFrame;
        Graphics3D_Present = Vulkan_Graphics3D_Present;
        Graphics3D_Destroy = Vulkan_Graphics3D_Destroy;
    }
    else if (gfx->m_ApiType == 1)
    {
        Graphics3D_WaitGPUIdle = GL_Graphics3D_WaitGPUIdle;
        Graphics3D_ExecuteCommandList = GL_Graphics3D_ExecuteCommandList;
        Graphics3D_ExecuteCommandLists = GL_Graphics3D_ExecuteCommandLists;
        Graphics3D_SetSyncInterval = GL_Graphics3D_SetSyncInterval;
        Graphics3D_PrepareNextFrame = GL_Graphics3D_PrepareNextFrame;
        Graphics3D_Present = GL_Graphics3D_Present;
        Graphics3D_Destroy = GL_Graphics3D_Destroy;
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
    PipelineLayout_LinkFunctions(gfx->m_context);
    PipelineState_LinkFunctions(gfx->m_context);
    CommandList_LinkFunctions(gfx->m_context);
    GPUFence_LinkFunctions(gfx->m_context);
    GPUSemaphore_LinkFunctions(gfx->m_context);
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
