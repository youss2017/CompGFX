#pragma once
#include "../core/egxcommon.hpp"
#include "../core/egxutil.hpp"
#include "../core/memory/egxmemory.hpp"
#include "../cmd/synchronization.hpp"
#include "../core/pipeline/egxsampler.hpp"
#include "../core/pipeline/egxframebuffer.hpp"
#include "../core/shaders/egxshaderset.hpp"
#include "../core/shaders/egxshader.hpp"

namespace egx {

	class VulkanSwapchain : public Synchronization {
	public:
		EGX_API VulkanSwapchain(ref<VulkanCoreInterface>& CoreInterface, void* GlfwWindowPtr, bool VSync, bool SetupImGui);
		VulkanSwapchain(VulkanSwapchain& copy) = delete;
		EGX_API VulkanSwapchain(VulkanSwapchain&& move) noexcept;
		EGX_API VulkanSwapchain& operator=(VulkanSwapchain& move) noexcept;
		EGX_API ~VulkanSwapchain();

		void EGX_API Acquire();
		void Present(const ref<Framebuffer>& Framebuffer, uint32_t ColorAttachmentId, uint32_t ViewIndex) {
			Present(Framebuffer->GetColorAttachment(ColorAttachmentId), ViewIndex);
		}
		// [Note]: Put your rendering finished semaphore in the SynchronizationContext object
		void EGX_API Present(const ref<Image>& Image, uint32_t ViewIndex);
		// [Note]: Put your rendering finished semaphore in the SynchronizationContext object
		void EGX_API Present();

		void EGX_API SetSyncInterval(bool VSync);
		// Uses Custom Size
		void EGX_API Resize(int width, int height);
		// Uses Window Size
		void EGX_API Resize();

	private:
		VkSurfaceFormatKHR GetSurfaceFormat();
		VkPresentModeKHR GetPresentMode();
		void CreateSwapchain(int width, int height);
		void CreateRenderPass();
		void CreatePipelineObjects();
		void CreatePipeline();
		uint32_t PresentInit();
		void PresentCommon(uint32_t frame, bool onlyimgui);

	public:
		VkSwapchainKHR Swapchain = nullptr;
		VkSurfaceKHR Surface = nullptr;
		std::vector<VkImage> Imgs;
		std::vector<VkImageView> Views;
		void* GlfwWindowPtr = nullptr;

	private:
		ref<VulkanCoreInterface> _core;
		VkRenderPass RenderPass = nullptr;
		bool _vsync;
		bool _imgui;
		int _width{};
		int _height{};
		VkSurfaceCapabilitiesKHR _capabilities{};
		std::vector<VkSurfaceFormatKHR> _surfaceFormats;
		std::vector<VkPresentModeKHR> _presentationModes;
		std::vector<VkFramebuffer> _framebuffers;
		ref<CommandPool> _pool;
		std::vector<VkCommandBuffer> _cmd;
		ref<Fence> _cmd_lock;
		ref<Semaphore> _acquire_lock;
		ref<Semaphore> _image_blit_lock;
		uint32_t _image_index = 0;
		bool _resize_swapchain_flag = false;
		
		Shader _builtin_vertex;
		Shader _builtin_fragment;
		ref<Sampler> _image_sampler;
		ref<SetPool> _descriptor_set_pool;
		VkDescriptorSetLayout _descriptor_layout = nullptr;
		std::vector<VkDescriptorSet> _descriptor_set;
		VkPipelineLayout _blit_layout = nullptr;
		VkPipeline _blit_gfx = nullptr;
		std::vector<VkImage> _last_updated_image;
	};

}
