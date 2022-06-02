#include "Swapchain.hpp"
#include "../../utils/Profiling.hpp"
#include "../../window/PlatformWindow.hpp"
#include "../backend/VulkanLoader.h"
#include "../../utils/common.hpp"
#include "../shaders/Shader.hpp"
#include "GUI.h"
#include <vector>
#include <climits>
#include <cassert>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "VkGraphicsCard.hpp"

/*
    Written: 9/26/2021
    Last Modified: 12/21/2021
    The swapchain is not thread safe.
    Goal of this file is to create and manage Vulkan Swapchain.
    - Must create/destroy swapchain
    - Get Next Frame
    - Copy Color Texture to Swapchain Image
*/

#if defined(_DEBUG)
#ifdef vkcheck
#undef vkcheck
#endif
#define vkcheck(x)\
{\
	VkResult ___vk_i_result____ = x;\
	if(___vk_i_result____  != VK_SUCCESS) {\
		std::string error = "Vulkan-Error: function ---> "; error += #x; error += " failed with error code " + __GetVkResultString(___vk_i_result____);\
		log_error(error.c_str(), __FILE__, __LINE__);\
	}\
}
#else
#define vkcheck(x) x
#endif

struct SwapchainPushblock {
    glm::ivec2 u_ShowColor;
    glm::vec2 zNear;
    glm::vec2 zFar;
    glm::vec2 u_Exposure;
} static pushblock;

static std::string __GetVkResultString(VkResult result)
	{
		if(VK_SUCCESS == result) return std::string("VK_SUCCESS");
		if(VK_NOT_READY == result) return std::string("VK_NOT_READY");
		if(VK_TIMEOUT == result) return std::string("VK_TIMEOUT");
		if(VK_EVENT_SET == result) return std::string("VK_EVENT_SET");
		if(VK_EVENT_RESET == result) return std::string("VK_EVENT_RESET");
		if(VK_INCOMPLETE == result) return std::string("VK_INCOMPLETE");
		if(VK_ERROR_OUT_OF_HOST_MEMORY == result) return std::string("VK_ERROR_OUT_OF_HOST_MEMORY");
		if(VK_ERROR_OUT_OF_DEVICE_MEMORY == result) return std::string("VK_ERROR_OUT_OF_DEVICE_MEMORY");
		if(VK_ERROR_INITIALIZATION_FAILED == result) return std::string("VK_ERROR_INITIALIZATION_FAILED");
		if(VK_ERROR_DEVICE_LOST == result) return std::string("VK_ERROR_DEVICE_LOST");
		if(VK_ERROR_MEMORY_MAP_FAILED == result) return std::string("VK_ERROR_MEMORY_MAP_FAILED");
		if(VK_ERROR_LAYER_NOT_PRESENT == result) return std::string("VK_ERROR_LAYER_NOT_PRESENT");
		if(VK_ERROR_EXTENSION_NOT_PRESENT == result) return std::string("VK_ERROR_EXTENSION_NOT_PRESENT");
		if(VK_ERROR_FEATURE_NOT_PRESENT == result) return std::string("VK_ERROR_FEATURE_NOT_PRESENT");
		if(VK_ERROR_INCOMPATIBLE_DRIVER == result) return std::string("VK_ERROR_INCOMPATIBLE_DRIVER");
		if(VK_ERROR_TOO_MANY_OBJECTS == result) return std::string("VK_ERROR_TOO_MANY_OBJECTS");
		if(VK_ERROR_FORMAT_NOT_SUPPORTED == result) return std::string("VK_ERROR_FORMAT_NOT_SUPPORTED");
		if(VK_ERROR_FRAGMENTED_POOL == result) return std::string("VK_ERROR_FRAGMENTED_POOL");
		if(VK_ERROR_UNKNOWN == result) return std::string("VK_ERROR_UNKNOWN");
		if(VK_ERROR_OUT_OF_POOL_MEMORY == result) return std::string("VK_ERROR_OUT_OF_POOL_MEMORY");
		if(VK_ERROR_INVALID_EXTERNAL_HANDLE == result) return std::string("VK_ERROR_INVALID_EXTERNAL_HANDLE");
		if(VK_ERROR_FRAGMENTATION == result) return std::string("VK_ERROR_FRAGMENTATION");
		if(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS == result) return std::string("VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS");
		if(VK_ERROR_SURFACE_LOST_KHR == result) return std::string("VK_ERROR_SURFACE_LOST_KHR");
		if(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR == result) return std::string("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
		if(VK_SUBOPTIMAL_KHR == result) return std::string("VK_SUBOPTIMAL_KHR");
		if(VK_ERROR_OUT_OF_DATE_KHR == result) return std::string("VK_ERROR_OUT_OF_DATE_KHR");
		if(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR == result) return std::string("VK_ERROR_INCOMPATIBLE_DISPLAY_KHR");
		if(VK_ERROR_VALIDATION_FAILED_EXT == result) return std::string("VK_ERROR_VALIDATION_FAILED_EXT");
		if(VK_ERROR_INVALID_SHADER_NV == result) return std::string("VK_ERROR_INVALID_SHADER_NV");
		if(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT == result) return std::string("VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT");
		if(VK_ERROR_NOT_PERMITTED_EXT == result) return std::string("VK_ERROR_NOT_PERMITTED_EXT");
		if(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT == result) return std::string("VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT");
		if(VK_THREAD_IDLE_KHR == result) return std::string("VK_THREAD_IDLE_KHR");
		if(VK_THREAD_DONE_KHR == result) return std::string("VK_THREAD_DONE_KHR");
		if(VK_OPERATION_DEFERRED_KHR == result) return std::string("VK_OPERATION_DEFERRED_KHR");
		if(VK_OPERATION_NOT_DEFERRED_KHR == result) return std::string("VK_OPERATION_NOT_DEFERRED_KHR");
		if(VK_PIPELINE_COMPILE_REQUIRED_EXT == result) return std::string("VK_PIPELINE_COMPILE_REQUIRED_EXT");
		if(VK_ERROR_OUT_OF_POOL_MEMORY_KHR == result) return std::string("VK_ERROR_OUT_OF_POOL_MEMORY_KHR");
		if(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR == result) return std::string("VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR");
		if(VK_ERROR_FRAGMENTATION_EXT == result) return std::string("VK_ERROR_FRAGMENTATION_EXT");
		if(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT == result) return std::string("VK_ERROR_INVALID_DEVICE_ADDRESS_EXT");
		if(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR == result) return std::string("VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR");
		if(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT == result) return std::string("VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT");
		if(VK_RESULT_MAX_ENUM == result) return std::string("VK_RESULT_MAX_ENUM");
		return std::string("UNDEFEINED or UNDETECTED VKRESULT! VkGraphicsCard.cpp");
	}

namespace vk
{

    GRAPHICS_API GraphicsSwapchain GraphicsSwapchain::Create(vk::VkContext context, float zNear, float zFar, uint32_t QueueFamilyIndex, VkFormat Format, PlatformWindow *Window, int BackBufferCount, int SyncInterval, bool UsingImGui)
    {
        GraphicsSwapchain gfxswap;
        gfxswap.mContext = context;
        gfxswap.m_QueueFamilyIndex = QueueFamilyIndex;
        gfxswap.m_BackBufferCount = BackBufferCount;
        gfxswap.m_Format = Format;
        gfxswap.m_SyncInterval = SyncInterval;
        gfxswap.m_Window = Window;
        gfxswap.m_ImageIndex = 0;
        gfxswap.m_UsingImGui = UsingImGui;
        gfxswap.zFar = zFar;
        gfxswap.zNear = zNear;
        assert(BackBufferCount > 0 && SyncInterval >= 0 && SyncInterval <= 4 && Window);
        /* Create Window Surface */
/*
#ifdef _WIN32
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        surfaceCreateInfo.hinstance = GetModuleHandleA(NULL);
        surfaceCreateInfo.hwnd = (HWND)Window->window_handle;
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(Instance, "vkCreateWin32SurfaceKHR");
        if (!vkCreateWin32SurfaceKHR)
        {
            log_error("Could not load vkCreateWin32SurfaceKHR function address. Maybe missing instance layer extension? or driver not updated.", __FILE__, __LINE__);
            Utils::Break();
        }
        vkCreateWin32SurfaceKHR(Instance, &surfaceCreateInfo, allocation_callback, &gfxswap.m_WindowSurface);
#else
        // Don't need to cast to GLWindow* since it is the first element.
        VkXlibSurfaceCreateInfoKHR surfaceCreateInfo{VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
        surfaceCreateInfo.dpy = *(Display**)Window->window_handle;
        vkCreateXlibSurfaceKHR(Instance, &surfaceCreateInfo, allocation_callback, &gfxswap.m_WindowSurface);
#endif
*/
        if(glfwCreateWindowSurface(context->instance, Window->GetWindow(), 0, &gfxswap.m_WindowSurface) != VK_SUCCESS) {
            logerror("Failed to create window surface in swapchian.");
        }
        /* Create Window Surface */

        // Create Render Pass
        VkAttachmentDescription ColorAttachment{};
        ColorAttachment.format = gfxswap.m_Format;
        ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference ColorReference;
        ColorReference.attachment = 0;
        ColorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription ColorSubpass{};
        ColorSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        ColorSubpass.colorAttachmentCount = 1;
        ColorSubpass.pColorAttachments = &ColorReference;
        VkRenderPassCreateInfo RenderPassCreateInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        RenderPassCreateInfo.attachmentCount = 1;
        RenderPassCreateInfo.pAttachments = &ColorAttachment;
        RenderPassCreateInfo.subpassCount = 1;
        RenderPassCreateInfo.pSubpasses = &ColorSubpass;
        if (vkCreateRenderPass(context->defaultDevice, &RenderPassCreateInfo, 0, &gfxswap.m_PresentPass) != VK_SUCCESS)
        {
            log_error("Could not create present render pass for swapchain! fatal error.", __FILE__, __LINE__);
            Utils::Break();
        }

        VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 1000.0f;
        samplerCreateInfo.maxAnisotropy = 16.0;
        vkCreateSampler(context->defaultDevice, &samplerCreateInfo, 0, &gfxswap.m_ColorTextureSampler);
        
        gfxswap.m_Swapchain = nullptr;
        gfxswap.CreateSwapchain(0);

        // 1) create pipeline
        VkDescriptorSetLayoutBinding SetLayoutBinding{};
        SetLayoutBinding.binding = 0;
        SetLayoutBinding.descriptorCount = 1;
        SetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        SetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo SetLayoutCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        SetLayoutCreateInfo.bindingCount = 1;
        SetLayoutCreateInfo.pBindings = &SetLayoutBinding;
        vkCreateDescriptorSetLayout(context->defaultDevice, &SetLayoutCreateInfo, 0, &gfxswap.m_SetLayout);

        VkPipelineLayoutCreateInfo LayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        LayoutCreateInfo.setLayoutCount = 1;
        LayoutCreateInfo.pSetLayouts = &gfxswap.m_SetLayout;
        LayoutCreateInfo.pushConstantRangeCount = 1;
        VkPushConstantRange range{};
        range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        range.offset = 0;
        range.size = sizeof(pushblock);
        LayoutCreateInfo.pPushConstantRanges = &range;
        vkCreatePipelineLayout(context->defaultDevice, &LayoutCreateInfo, 0, &gfxswap.m_Layout);

        // create descriptor pool
        std::array<VkDescriptorPoolSize, 2> PoolSizes;
        PoolSizes[0].descriptorCount = BackBufferCount;
        PoolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        PoolSizes[1].descriptorCount = BackBufferCount;
        PoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        VkDescriptorPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolCreateInfo.maxSets = BackBufferCount + 1;
        poolCreateInfo.poolSizeCount = PoolSizes.size();
        poolCreateInfo.pPoolSizes = PoolSizes.data();
        vkCreateDescriptorPool(context->defaultDevice, &poolCreateInfo, 0, &gfxswap.m_DescriptorPool);
        // create descriptor set
        gfxswap.m_DescriptorSets = new VkDescriptorSet[BackBufferCount];
        for (int i = 0; i < BackBufferCount; i++)
        {
            VkDescriptorSetAllocateInfo AllocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            AllocateInfo.descriptorPool = gfxswap.m_DescriptorPool;
            AllocateInfo.descriptorSetCount = 1;
            AllocateInfo.pSetLayouts = &gfxswap.m_SetLayout;
            vkcheck(vkAllocateDescriptorSets(context->defaultDevice, &AllocateInfo, &gfxswap.m_DescriptorSets[i]));
        }

        // create command objects
        VkCommandPoolCreateInfo CommandPoolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
        CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkcheck(vkCreateCommandPool(context->defaultDevice, &CommandPoolCreateInfo, 0, &gfxswap.m_CommandPool));

        VkCommandBufferAllocateInfo AllocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        AllocateInfo.commandPool = gfxswap.m_CommandPool;
        AllocateInfo.commandBufferCount = BackBufferCount;
        AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        gfxswap.m_CommandBuffers.resize(BackBufferCount);
        vkcheck(vkAllocateCommandBuffers(context->defaultDevice, &AllocateInfo, gfxswap.m_CommandBuffers.data()));

        gfxswap.CreatePipeline(0);

        gfxswap.m_QueuePresentWaitSemphores.resize(gfxswap.m_BackBufferCount); 
        gfxswap.m_CmdFences.resize(gfxswap.m_BackBufferCount);
        for (unsigned int i = 0; i < gfxswap.m_BackBufferCount; i++)
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            vkCreateSemaphore(context->defaultDevice, &semaphoreCreateInfo, nullptr, &gfxswap.m_QueuePresentWaitSemphores[i]);
            VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            vkCreateFence(context->defaultDevice, &fenceCreateInfo, nullptr, &gfxswap.m_CmdFences[i]);
        }

        return gfxswap;
    }

    GRAPHICS_API void GraphicsSwapchain::CreateSwapchain(VkAllocationCallbacks *allocation_callback)
    {
        // Get information for SwapChain
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mContext->card.handle, m_WindowSurface, &surfaceCapabilities);
        uint32_t surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->card.handle, m_WindowSurface, &surfaceFormatCount, NULL);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->card.handle, m_WindowSurface, &surfaceFormatCount, surfaceFormats.data());
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(mContext->card.handle, m_WindowSurface, &presentModeCount, NULL);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(mContext->card.handle, m_WindowSurface, &presentModeCount, presentModes.data());

        if (m_BackBufferCount < surfaceCapabilities.minImageCount)
        {
            char buf[150];
            buf[0] = 0;
            strcat(buf, "Swapchain BackBufferCount is not enough for the graphics card. Setting BackBufferCount to default. ");
            char temp[15];
            temp[0] = 0;
            //itoa(surfaceCapabilities.minImageCount, temp, 10);
            strcat(buf, temp);
            logwarning(buf);
        }
        Utils::ClampValues(m_BackBufferCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);

        // Check SwapChain Surface Format is supported
        bool FormatSupported = false;
        VkColorSpaceKHR ColorSpace{};
        for (const auto &sf : surfaceFormats)
        {
            if (sf.format == m_Format)
            {
                ColorSpace = sf.colorSpace;
                FormatSupported = true;
                break;
            }
        }

        if (!FormatSupported)
        {
            logalert("Surface format is not supported on this graphics card. Could not create swapchain. Defaulting to the first supported surface format.");
            if(surfaceFormats.size() > 0) {
                m_Format = surfaceFormats[0].format;
                ColorSpace = surfaceFormats[0].colorSpace;
            } else {
                logerror("No surface format found!");
                Utils::FatalBreak();
            }
        }

        // Choose PresentMode
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        if (m_SyncInterval > 0)
        {
            for (const auto &pm : presentModes)
            {
                if (pm == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
                {
                    presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                    break;
                }
                else if (pm == VK_PRESENT_MODE_FIFO_KHR)
                {
                    presentMode = VK_PRESENT_MODE_FIFO_KHR;
                    break;
                }
            }
        }

        // Choose Swap Extent
        VkExtent2D swapExtend2d;
        if (surfaceCapabilities.currentExtent.width == UINT_MAX)
        {
            swapExtend2d = surfaceCapabilities.currentExtent;
        }
        else
        {
            swapExtend2d.width = m_Window->GetWidth();
            swapExtend2d.height = m_Window->GetHeight();
        }
        Utils::ClampValues(swapExtend2d.width, surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.minImageExtent.width);
        Utils::ClampValues(swapExtend2d.height, surfaceCapabilities.maxImageExtent.height, surfaceCapabilities.minImageExtent.height);
        m_FramebufferWidth = swapExtend2d.width;
        m_FramebufferHeight = swapExtend2d.height;

        VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        createInfo.surface = m_WindowSurface;
        createInfo.minImageCount = m_BackBufferCount;
        createInfo.imageFormat = m_Format;
        createInfo.imageColorSpace = ColorSpace;
        createInfo.imageExtent = swapExtend2d;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.clipped = VK_TRUE;

        VkBool32 supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(mContext->card.handle, m_QueueFamilyIndex, m_WindowSurface, &supported);
        if (supported == VK_TRUE)
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        else
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = m_Swapchain;
        Loader_LoadVulkan();
        Loader_LoadDevice(mContext->defaultDevice);
        VkResult result = vkCreateSwapchainKHR(mContext->defaultDevice, &createInfo, allocation_callback, &m_Swapchain);

        if (result != VK_SUCCESS)
        {
            log_error("Could not create Vulkan Swap chain!", __FILE__, __LINE__);
            Utils::Break();
        }

        if (!m_Framebuffers)
            m_Framebuffers = new VkFramebuffer[m_BackBufferCount];
        if (!m_Views)
            m_Views = new VkImageView[m_BackBufferCount];

        if (!m_SwapchainImages)
            m_SwapchainImages = new VkImage[m_BackBufferCount];
        vkGetSwapchainImagesKHR(mContext->defaultDevice, m_Swapchain, reinterpret_cast<uint32_t *>(&m_BackBufferCount), m_SwapchainImages);

        for (uint32_t i = 0; i < m_BackBufferCount; i++)
        {
            VkImageViewCreateInfo viewCreateInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            viewCreateInfo.image = m_SwapchainImages[i];
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = m_Format;
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = 1;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.layerCount = 1;
            VkImageView view;
            if (vkCreateImageView(mContext->defaultDevice, &viewCreateInfo, allocation_callback, &view) != VK_SUCCESS)
            {
                log_error("Could not create swapchain image view.", __FILE__, __LINE__);
                Utils::Break();
            }
            m_Views[i] = view;
            VkFramebufferCreateInfo fbCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fbCreateInfo.renderPass = m_PresentPass;
            fbCreateInfo.attachmentCount = 1;
            VkImageView framebufferAttachments[1] = {view};
            fbCreateInfo.pAttachments = framebufferAttachments;
            fbCreateInfo.width = swapExtend2d.width;
            fbCreateInfo.height = swapExtend2d.height;
            fbCreateInfo.layers = 1;
            VkFramebuffer framebuffer;
            vkcheck(vkCreateFramebuffer(mContext->defaultDevice, &fbCreateInfo, allocation_callback, &framebuffer));
            m_Framebuffers[i] = framebuffer;
        }

        // Transistion Swapchain images into VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to stop validation errors
        VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        VkCommandPool pool;
        vkCreateCommandPool(mContext->defaultDevice, &poolCreateInfo, nullptr, &pool);
        VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(mContext->defaultDevice, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        for (uint32_t i = 0; i < m_BackBufferCount; i++)
        {
            VkImageMemoryBarrier barrier;
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_SwapchainImages[i];
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        vkQueueSubmit(mContext->defaultQueue, 1, &submitInfo, nullptr);
        vkQueueWaitIdle(mContext->defaultQueue);
        vkDestroyCommandPool(mContext->defaultDevice, pool, nullptr);

        // initalize ImGui
        if (m_UsingImGui && !m_GuiInitalized)
        {
            Gui_VKInitalizeImGui(mContext->instance, allocation_callback, mContext->card.handle, m_QueueFamilyIndex, m_BackBufferCount, mContext->defaultDevice, m_PresentPass, m_Window);
            m_GuiInitalized = true;
        }
    }

    GRAPHICS_API void GraphicsSwapchain::CreatePipeline(VkAllocationCallbacks *allocation_callback)
    {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        // 1) compile shader source code into spirv
        Shader vertex = Shader("assets/shaders/postprocess/swapchain.vert");
        Shader fragment = Shader("assets/shaders/postprocess/swapchain.frag");

        VkPipelineShaderStageCreateInfo ShaderStages[2]{};
        ShaderStages[0].sType = ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[0].pName = "main";
        ShaderStages[0].module = vertex.GetShader();
        ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStages[1].pName = "main";
        ShaderStages[1].module = fragment.GetShader();
        ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = ShaderStages;

        VkPipelineVertexInputStateCreateInfo VertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VertexInputState.vertexBindingDescriptionCount = 0;
        VertexInputState.pVertexBindingDescriptions = NULL;
        VertexInputState.vertexAttributeDescriptionCount = 0;
        VertexInputState.pVertexAttributeDescriptions = NULL;
        pipelineCreateInfo.pVertexInputState = &VertexInputState;

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipelineCreateInfo.pInputAssemblyState = &InputAssemblyState;

        VkViewport viewport;
        viewport.x = viewport.y = 0;
        viewport.width = m_Window->GetWidth();
        viewport.height = m_Window->GetHeight();
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor;
        scissor.offset.x = scissor.offset.y = 0;
        scissor.extent.width = m_Window->GetWidth();
        scissor.extent.height = m_Window->GetHeight();

        VkPipelineViewportStateCreateInfo ViewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        ViewportState.viewportCount = 1;
        ViewportState.scissorCount = 1;
        ViewportState.pViewports = &viewport;
        ViewportState.pScissors = &scissor;
        pipelineCreateInfo.pViewportState = &ViewportState;

        VkPipelineRasterizationStateCreateInfo RasterizeState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        RasterizeState.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizeState.cullMode = VK_CULL_MODE_NONE;
        RasterizeState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        RasterizeState.lineWidth = 1.0f;
        pipelineCreateInfo.pRasterizationState = &RasterizeState;

        VkPipelineMultisampleStateCreateInfo MultisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        MultisampleState.minSampleShading = 1.0f;
        pipelineCreateInfo.pMultisampleState = &MultisampleState;

        VkPipelineDepthStencilStateCreateInfo DepthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        DepthStencilState.depthTestEnable = VK_FALSE;
        DepthStencilState.depthWriteEnable = VK_TRUE;
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
        pipelineCreateInfo.pDepthStencilState = &DepthStencilState;

        VkPipelineColorBlendAttachmentState ColorBlend0{};
        ColorBlend0.blendEnable = VK_FALSE;
        ColorBlend0.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo ColorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        ColorBlendState.attachmentCount = 1;
        ColorBlendState.pAttachments = &ColorBlend0;
        pipelineCreateInfo.pColorBlendState = &ColorBlendState;

        pipelineCreateInfo.layout = m_Layout;
        pipelineCreateInfo.renderPass = m_PresentPass;

        VkGraphicsPipelineCreateInfo pipelineDepthCreateInfo = pipelineCreateInfo;

        VkPipelineShaderStageCreateInfo ShaderDepthStages[2]{};
        ShaderDepthStages[0].sType = ShaderDepthStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderDepthStages[0].pName = "main";
        ShaderDepthStages[0].module = vertex.GetShader();
        ShaderDepthStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderDepthStages[1].pName = "main";
        ShaderDepthStages[1].module = fragment.GetShader();
        ShaderDepthStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineDepthCreateInfo.pStages = ShaderDepthStages;

        if (m_PresentPipeline)
            vkDestroyPipeline(mContext->defaultDevice, m_PresentPipeline, nullptr);

        vkCreateGraphicsPipelines(mContext->defaultDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, allocation_callback, &m_PresentPipeline);
    }

    GRAPHICS_API VkResult GraphicsSwapchain::PrepareNextFrame(uint64_t TimeOut, VkSemaphore NextFrameReadySemaphore, VkFence NextFrameReadyFence, uint32_t* pOutImageIndex)
    {
        PROFILE_FUNCTION();
        if (m_Window->IsWindowMinimized())
            return VK_TIMEOUT;
        VkResult result = vkAcquireNextImageKHR(mContext->defaultDevice, m_Swapchain, TimeOut, NextFrameReadySemaphore, NextFrameReadyFence, &m_ImageIndex);
        if (result != VK_SUCCESS)
        {
            // Wait until swapchain is not used in-flight
            vkQueueWaitIdle(mContext->defaultQueue); // TODO: Fix validation layer errors when resizing
            // the swapchain is to old, must be recreated.
            // Before creating a new swapchain destroy views and framebuffers
            for (uint32_t i = 0; i < m_BackBufferCount; i++)
            {
                vkDestroyImageView(mContext->defaultDevice, m_Views[i], 0);
                vkDestroyFramebuffer(mContext->defaultDevice, m_Framebuffers[i], 0);
            }
            CreateSwapchain(0);
            // Recreate Pipeline since viewport/scissor have changed
            CreatePipeline(0);
            vkAcquireNextImageKHR(mContext->defaultDevice, m_Swapchain, TimeOut, NextFrameReadySemaphore, NextFrameReadyFence, &m_ImageIndex);
        }
        if (m_UsingImGui)
            Gui_VKBeginGUIFrame();
        if(pOutImageIndex)
            *pOutImageIndex = m_ImageIndex;
        return VK_SUCCESS;
    }

    GRAPHICS_API void GraphicsSwapchain::Present(VkImage ColorTexture, VkImageView ColorTextureView, VkImageLayout ImageLayout, uint32_t WaitSemaphoreCount, VkSemaphore* pWaitSemaphores, bool DepthPipeline)
    {
        PROFILE_FUNCTION();
        {
            PROFILE_SCOPE("[Vulkan] Swapchain Present Pass");
            vkWaitForFences(mContext->defaultDevice, 1, &m_CmdFences[m_ImageIndex], true, 1e9);
            vkResetFences(mContext->defaultDevice, 1, &m_CmdFences[m_ImageIndex]);
            VkCommandBufferBeginInfo CmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            CmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkResetCommandBuffer(m_CommandBuffers[m_ImageIndex], 0);
            vkBeginCommandBuffer(m_CommandBuffers[m_ImageIndex], &CmdBeginInfo);
            vk::Gfx_InsertDebugLabel(mContext->defaultDevice, m_CommandBuffers[m_ImageIndex], m_ImageIndex, "Swapchain Present", 1.0, 1.0, 1.0);

            VkRenderPassBeginInfo PassBeginInfo;
            PassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            PassBeginInfo.pNext = NULL;
            PassBeginInfo.renderPass = m_PresentPass;
            PassBeginInfo.framebuffer = m_Framebuffers[m_ImageIndex];
            PassBeginInfo.renderArea.offset = {0, 0};
            PassBeginInfo.renderArea.extent = {m_FramebufferWidth, m_FramebufferHeight};
            PassBeginInfo.clearValueCount = 0;
            PassBeginInfo.pClearValues = NULL;

            // Set Image to DescriptorSet
            VkDescriptorImageInfo ImageWriteInfo;
            ImageWriteInfo.sampler = m_ColorTextureSampler;
            ImageWriteInfo.imageView = ColorTextureView;
            ImageWriteInfo.imageLayout = ImageLayout;
            VkWriteDescriptorSet DescriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            DescriptorWrite.dstSet = m_DescriptorSets[m_ImageIndex];
            DescriptorWrite.descriptorCount = 1;
            DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            DescriptorWrite.pImageInfo = &ImageWriteInfo;
            // TODO: Avoid doing this every frame.
            {
                PROFILE_SCOPE("[Vulkan] Updating Presenting Descriptor Set");
                vkUpdateDescriptorSets(mContext->defaultDevice, 1, &DescriptorWrite, 0, NULL);
            }

            vkCmdBeginRenderPass(m_CommandBuffers[m_ImageIndex], &PassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(m_CommandBuffers[m_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PresentPipeline);
            vkCmdBindDescriptorSets(m_CommandBuffers[m_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, 0, 1, &m_DescriptorSets[m_ImageIndex], 0, NULL);
            if (DepthPipeline) {
                pushblock.u_ShowColor[0] = 0;
                pushblock.zNear[0] = zNear;
                pushblock.zFar[0] = zFar;
            }
            else {
                pushblock.u_ShowColor[0] = 1;
                pushblock.u_Exposure[0] = mExposure;
            }
            vkCmdPushConstants(m_CommandBuffers[m_ImageIndex], m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushblock), &pushblock);
            vkCmdDraw(m_CommandBuffers[m_ImageIndex], 6, 1, 0, 0);
            vk::Gfx_EndDebugLabel(mContext->defaultDevice, m_CommandBuffers[m_ImageIndex]);
        }

        {
            PROFILE_SCOPE("[Vulkan] Swapchain ImGui Recording.");
            vk::Gfx_InsertDebugLabel(mContext->defaultDevice, m_CommandBuffers[m_ImageIndex], m_ImageIndex, "ImGui Commands", 0.0, 1.0);
            if (m_UsingImGui)
                Gui_VKEndGUIFrame(m_CommandBuffers[m_ImageIndex]);
            vk::Gfx_EndDebugLabel(mContext->defaultDevice, m_CommandBuffers[m_ImageIndex]);
        }

        vkCmdEndRenderPass(m_CommandBuffers[m_ImageIndex]);

        vkEndCommandBuffer(m_CommandBuffers[m_ImageIndex]);

        VkPipelineStageFlags pWaitDstStageMask[60];
        for(uint32_t i = 0; i < WaitSemaphoreCount; i++)
            pWaitDstStageMask[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        
        VkSubmitInfo SubmitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        SubmitInfo.waitSemaphoreCount = WaitSemaphoreCount;
        SubmitInfo.pWaitSemaphores = pWaitSemaphores;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &m_CommandBuffers[m_ImageIndex];
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = &m_QueuePresentWaitSemphores[m_ImageIndex];
        SubmitInfo.pWaitDstStageMask = pWaitDstStageMask;

        vkQueueSubmit(mContext->defaultQueue, 1, &SubmitInfo, m_CmdFences[m_ImageIndex]);

        VkPresentInfoKHR PresentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores = &m_QueuePresentWaitSemphores[m_ImageIndex];
        PresentInfo.swapchainCount = 1;
        PresentInfo.pSwapchains = &m_Swapchain;
        PresentInfo.pImageIndices = &m_ImageIndex;
        
        {
            PROFILE_SCOPE("[Vulkan] Swapchain vkQueuePresentKHR");
            VkResult present_status = vkQueuePresentKHR(mContext->defaultQueue, &PresentInfo);
            if (present_status != VK_SUCCESS)
            {
                // Did the window minimize?
                if (m_Window->IsWindowMinimized())
                    goto NoResize;
                // Wait until swapchain is not used in-flight
                vkQueueWaitIdle(mContext->defaultQueue); // TODO: Fix validation layer errors when resizing
                // the swapchain is to old, must be recreated.
                // Before creating a new swapchain destroy views and framebuffers
                for (uint32_t i = 0; i < m_BackBufferCount; i++)
                {
                    vkDestroyImageView(mContext->defaultDevice, m_Views[i], 0);
                    vkDestroyFramebuffer(mContext->defaultDevice, m_Framebuffers[i], 0);
                }
                CreateSwapchain(0);
                // Recreate Pipeline since viewport/scissor have changed
                CreatePipeline(0);
            }
        }
        
        NoResize:
        return;
    }

    GRAPHICS_API void GraphicsSwapchain::Destroy()
    {
        PROFILE_FUNCTION();
        vkQueueWaitIdle(mContext->defaultQueue); // make sure swapchain is not in-flight
        vkDestroyPipeline(mContext->defaultDevice, m_PresentPipeline, 0);
        vkDestroyRenderPass(mContext->defaultDevice, m_PresentPass, 0);
        vkDestroyDescriptorPool(mContext->defaultDevice, m_DescriptorPool, 0);
        vkDestroyDescriptorSetLayout(mContext->defaultDevice, m_SetLayout, 0);
        delete[] m_DescriptorSets;
        vkDestroyPipelineLayout(mContext->defaultDevice, m_Layout, 0);
        delete[] m_SwapchainImages;
        for (uint32_t i = 0; i < m_BackBufferCount; i++)
        {
            vkDestroySemaphore(mContext->defaultDevice, m_QueuePresentWaitSemphores[i], nullptr);
            vkDestroyFence(mContext->defaultDevice, m_CmdFences[i], nullptr);
            vkDestroyImageView(mContext->defaultDevice, m_Views[i], 0);
            vkDestroyFramebuffer(mContext->defaultDevice, m_Framebuffers[i], 0);
        }
        vkDestroySwapchainKHR(mContext->defaultDevice, m_Swapchain, 0);
        vkDestroySurfaceKHR(mContext->instance, m_WindowSurface, 0);
        vkDestroySampler(mContext->defaultDevice, m_ColorTextureSampler, 0);
        if (m_UsingImGui)
            Gui_VKDestroyImGui(0);
        delete[] m_Framebuffers;
        delete[] m_Views;
        vkDestroyCommandPool(mContext->defaultDevice, m_CommandPool, 0);
    }

}