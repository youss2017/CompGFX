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
            static GraphicsSwapchain Create(VkInstance Instance, VkAllocationCallbacks* allocation_callback, VkPhysicalDevice PhysicalDevice, VkDevice Device, VkQueue Queue, uint32_t QueueFamilyIndex, VkFormat Format, PlatformWindow *Window, int BackBufferCount, int SyncInterval, bool UsingImGui);
            // pNextImageIndex will be between 0 and BackBufferCount, When submitting commands to command queue, make sure to wait for pSwapchainReadySemaphore
            VkResult PrepareNextFrame(uint64_t TimeOut, VkSemaphore NextFrameReadySemaphore, VkFence NextFrameReadyFence, uint32_t* pOutImageIndex);
            // Image Layout should be VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pRenderCompleteSemaphores is a list of semaphores to wait so that gpu finishes rendering the frame before showing the image
            void Present(VkImage ColorTexture, VkImageView ColorTextureView, VkImageLayout ImageLayout, uint32_t WaitSemaphoreCount, VkSemaphore* pWaitSemaphores, bool DepthPipeline);
            void Destroy();

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
            inline VkRenderPass GetRenderPass() {
                return m_PresentPass;
            }

        private:    
            void CreateSwapchain(VkAllocationCallbacks* allocation_callback);
            void CreatePipeline(VkAllocationCallbacks* allocation_callback);
    };

}