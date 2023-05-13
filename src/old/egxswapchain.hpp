#pragma once
#include "../core/egxengine.hpp"
#include "../core/egxutil.hpp"
#include "../core/memory/egxmemory.hpp"
#include "../cmd/synchronization.hpp"
#include "../core/pipeline/egxsampler.hpp"
#include "../core/pipeline/egxframebuffer.hpp"
#include "../core/shaders/egxshaderset.hpp"
#include "../core/shaders/egxshader2.hpp"

namespace egx {

	class SwapchainControllerx : public ISynchronization {
	public:
		EGX_API SwapchainController(const DeviceCtx* pCtx, int maxFramesInFlight, bool initImGui, bool vSync);
		SwapchainController(SwapchainController& copy) = delete;
		EGX_API SwapchainController& operator=(SwapchainController& move) = delete;
		EGX_API SwapchainController(SwapchainController&& move) noexcept;
		EGX_API ~SwapchainController();

		void EGX_API Acquire();
		void Present(const ref<Framebuffer>& Framebuffer, uint32_t ColorAttachmentId, uint32_t ViewIndex) {
			Present(Framebuffer->GetColorAttachment(ColorAttachmentId), ViewIndex);
		}
		// [Note]: Put your rendering finished semaphore in the SynchronizationContext object
		void EGX_API Present(const ref<Image>& Image, uint32_t ViewIndex);
		// [Note]: Put your rendering finished semaphore in the SynchronizationContext object
		void EGX_API Present();

		void EGX_API SetSyncInterval(bool vsync);
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

	private:
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
		ref<Semaphore> _acquire_lock;
		uint32_t _image_index = 0;
		bool _recreate_swapchain_flag = false;
		std::vector<VkImage> Imgs;
		std::vector<VkImageView> Views;
		//std::vector<egx::ref<Image>> BackBuffers;
		VkSurfaceKHR Surface = nullptr;
		void* GlfwWindowPtr = nullptr;

		VkShaderModule _builtin_vertex_module;
		VkShaderModule _builtin_fragment_module;

		ref<Sampler> _image_sampler;
		ref<SetPool> _descriptor_set_pool;
		VkDescriptorSetLayout _descriptor_layout = nullptr;
		std::vector<VkDescriptorSet> _descriptor_set;
		VkPipelineLayout _blit_layout = nullptr;
		VkPipeline _blit_gfx = nullptr;
		std::vector<VkImage> _last_updated_image;
	};

}
