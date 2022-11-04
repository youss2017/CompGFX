#pragma once
#include "../core/egxcommon.hpp"
#include "../core/egxutil.hpp"
#include "../core/memory/egxmemory.hpp"
#include "../cmd/synchronization.hpp"

namespace egx {

	class VulkanSwapchain {
	public:
		EGX_API VulkanSwapchain(ref<VulkanCoreInterface>& CoreInterface, void* GlfwWindowPtr, bool VSync, bool SetupImGui, bool ClearSwapchain);
		VulkanSwapchain(VulkanSwapchain& copy) = delete;
		EGX_API VulkanSwapchain(VulkanSwapchain&& move) noexcept;
		EGX_API VulkanSwapchain& operator=(VulkanSwapchain& move) noexcept;
		EGX_API ~VulkanSwapchain();

		void EGX_API Acquire();
		// [Note]: Put your rendering finished semaphore in the SynchronizationCollection object
		void EGX_API Present(const ref<Image>& image, uint32_t viewIndex);
		// [Note]: Put your rendering finished semaphore in the SynchronizationCollection object
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
		uint32_t PresentInit();
		void PresentCommon(uint32_t frame);

	public:
		VkSwapchainKHR Swapchain = nullptr;
		VkSurfaceKHR Surface = nullptr;
		std::vector<VkImage> Imgs;
		std::vector<VkImageView> Views;
		void* GlfwWindowPtr = nullptr;
		SynchronizationCollection Synchronization;

	private:
		ref<VulkanCoreInterface> _core;
		VkRenderPass RenderPass = nullptr;
		bool _vsync;
		bool _imgui;
		bool _clearswapchain;
		int _width{};
		int _height{};
		uint32_t _current_swapchain_frame = 0;
		VkSurfaceCapabilitiesKHR _cabilities{};
		std::vector<VkSurfaceFormatKHR> _surfaceFormats;
		std::vector<VkPresentModeKHR> _presentationModes;
		std::vector<VkFramebuffer> _framebuffers;
		ref<CommandPool> _pool;
		std::vector<VkCommandBuffer> _cmd;
		ref<Fence> _poollock;
		VkDescriptorPool _imguipool = nullptr;
	};

}
