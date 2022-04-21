#pragma once
#include "backend_base.h"
#include <vulkan/vulkan.h>
#include <vector>

class PlatformWindow;

namespace vk {

    class GraphicsSwapchain {
        public:
            // Do not use SRGB VkFormat since the fragment shader already applies gamma correction at level 2.2
            GRAPHICS_API static GraphicsSwapchain Create(vk::VkContext context, float zNear, float zFar, uint32_t QueueFamilyIndex, VkFormat Format, PlatformWindow *Window, int BackBufferCount, int SyncInterval, bool UsingImGui);
            // pNextImageIndex will be between 0 and BackBufferCount, When submitting commands to command queue, make sure to wait for pSwapchainReadySemaphore
            GRAPHICS_API VkResult PrepareNextFrame(uint64_t TimeOut, VkSemaphore NextFrameReadySemaphore, VkFence NextFrameReadyFence, uint32_t* pOutImageIndex);
            // Image Layout should be VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pRenderCompleteSemaphores is a list of semaphores to wait so that gpu finishes rendering the frame before showing the image
            GRAPHICS_API void Present(VkImage ColorTexture, VkImageView ColorTextureView, VkImageLayout ImageLayout, uint32_t WaitSemaphoreCount, VkSemaphore* pWaitSemaphores, bool DepthPipeline);
            GRAPHICS_API void Destroy();

            GRAPHICS_API void SetExposure(float exposure) {
                mExposure = exposure;
            }

            // Get Actual backbuffercount
            inline uint32_t GetBackBufferCount() { return m_BackBufferCount; }

        private:
            float mExposure = 1.0f;
            float zNear;
            float zFar;
            VkPipeline m_PresentPipeline;
            VkRenderPass m_PresentPass;
            VkDescriptorSetLayout m_SetLayout;
            VkDescriptorPool m_DescriptorPool;
            VkDescriptorSet* m_DescriptorSets = NULL;
            VkPipelineLayout m_Layout;

            VkSurfaceKHR m_WindowSurface;
            uint32_t m_ImageIndex;
            VkImage* m_SwapchainImages = NULL;
            std::vector<VkSemaphore> m_QueuePresentWaitSemphores;
            std::vector<VkFence> m_CmdFences;
            VkSwapchainKHR m_Swapchain;
            VkFramebuffer* m_Framebuffers = NULL;
            VkImageView* m_Views = NULL;

            uint32_t m_FramebufferWidth, m_FramebufferHeight;

            VkSampler m_ColorTextureSampler; // Sampler used to render the color texture
            
            VkCommandPool m_CommandPool;
            std::vector<VkCommandBuffer> m_CommandBuffers;
            
            bool m_UsingImGui, m_GuiInitalized = false;

            vk::VkContext mContext;
            uint32_t m_QueueFamilyIndex;
            uint32_t m_BackBufferCount;
            VkFormat m_Format;
            int m_SyncInterval;
            PlatformWindow* m_Window;

        public:
            inline VkRenderPass GetRenderPass() {
                return m_PresentPass;
            }

        private:    
            GRAPHICS_API void CreateSwapchain(VkAllocationCallbacks* allocation_callback);
            GRAPHICS_API void CreatePipeline(VkAllocationCallbacks* allocation_callback);
    };

}