#pragma once
#include "../memory/vulkan_memory.h"
#include "backend_base.h"
#include <vulkan/vulkan.h>
#include <vector>

class PlatformWindow;

namespace vk {

    class GraphicsSwapchain {
        public:
            // Do not use SRGB VkFormat since the fragment shader already applies gamma correction at level 2.2
            static GraphicsSwapchain Create(VkInstance Instance, VkAllocationCallbacks* allocation_callback, VkPhysicalDevice PhysicalDevice, VkDevice Device, VkQueue Queue, uint32_t QueueFamilyIndex, VkFormat Format, PlatformWindow *Window, int BackBufferCount, int SyncInterval, _FrameInformation** pOutFrameInfo, bool UsingImGui);
            // pNextImageIndex will be between 0 and BackBufferCount, When submitting commands to command queue, make sure to wait for pSwapchainReadySemaphore
            bool PrepareNextFrame(uint32_t* pNextImageIndex, VkSemaphore* pSwapchainReadySemaphore);
            // Image Layout should be VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pRenderCompleteSemaphores is a list of semaphores to wait so that gpu finishes rendering the frame before showing the image
            void Present(VkImage ColorTexture, VkImageView ColorTextureView, VkImageLayout ImageLayout, uint32_t WaitSemaphoreCount, VkSemaphore* pWaitSemaphores, bool DepthPipeline);
            void Destroy();

            inline uint32_t GetLastFrameIndex() { return m_LastFrameIndex; }
            // Get Actual backbuffercount
            inline uint32_t GetBackBufferCount() { return m_BackBufferCount; }

        private:
            VkShaderModule m_VertexModule, m_FragmentModule;
            VkPipeline m_PresentPipeline, m_PresentDepthPipeline;
            VkRenderPass m_PresentPass;
            VkDescriptorSetLayout m_SetLayout;
            VkDescriptorPool m_DescriptorPool;
            VkDescriptorSet* m_DescriptorSets = NULL;
            VkPipelineLayout m_Layout;

            VkSurfaceKHR m_WindowSurface;
            uint32_t m_SwapchainImageIndex;
            VkImage* m_SwapchainImages = NULL;
            VkSwapchainKHR m_Swapchain;
            VkFramebuffer* m_Framebuffers = NULL;
            VkImageView* m_Views = NULL;

            uint32_t m_FramebufferWidth, m_FramebufferHeight;

            // Frame Sync
            uint32_t m_LastFrameIndex;
            uint64_t m_FrameCount;
            // The is size of the following is equal to m_BackBufferCount
            VkFence *m_FrameFences  = NULL;
            //VkSemaphore *m_FrameSemaphores  = NULL;
            VkSemaphore *m_FrameSemaphores = nullptr;
            VkSemaphore *m_FrameSwapchainCompleteSemaphores  = NULL;

            VkSampler m_ColorTextureSampler; // Sampler used to render the color texture
            
            VkCommandPool m_CommandPool;
            VkCommandBuffer* m_CommandBuffers = NULL;
            
            bool m_UsingImGui, m_GuiInitalized = false;

            VkInstance m_Instance;
            VkAllocationCallbacks* m_allocation_callback;
            VkDevice m_Device;
            VkQueue m_Queue;
            VkPhysicalDevice m_PhysicalDevice;
            uint32_t m_QueueFamilyIndex;
            uint32_t m_BackBufferCount;
            VkFormat m_Format;
            int m_SyncInterval;
            PlatformWindow* m_Window;

        public:
            _FrameInformation* m_FrameInfo;
            inline VkRenderPass GetRenderPass() {
                return m_PresentPass;
            }

        private:
            void CreateSwapchain(VkAllocationCallbacks* allocation_callback);
            void CreatePipeline(VkAllocationCallbacks* allocation_callback);

    };

}