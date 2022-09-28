#pragma once
#include "../core/egxcommon.hpp"
#include "../core/egxutil.hpp"
#include "../core/memory/egxmemory.hpp"

namespace egx {

	class VulkanSwapchain {
	public:
		EGX_API VulkanSwapchain(ref<VulkanCoreInterface>& CoreInterface, void* GlfwWindowPtr, bool VSync, bool SetupImGui, bool ClearSwapchain);
		VulkanSwapchain(VulkanSwapchain& copy) = delete;
		EGX_API VulkanSwapchain(VulkanSwapchain&& move) noexcept;
		EGX_API VulkanSwapchain& operator=(VulkanSwapchain& move) noexcept;
		EGX_API ~VulkanSwapchain();

		void EGX_API Acquire(bool block, VkSemaphore ReadySemaphore, VkFence ReadyFence);
		void EGX_API Present(ref<Image>& image, uint32_t viewIndex, const std::vector<VkSemaphore>& WaitSemaphores = {});
		void EGX_API Present(const std::vector<VkSemaphore>& WaitSemaphores = {});

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

	public:
		VkSwapchainKHR Swapchain = nullptr;
		VkSurfaceKHR Surface = nullptr;
		std::vector<VkImage> Imgs;
		std::vector<VkImageView> Views;
		void* GlfwWindowPtr = nullptr;

	private:
		ref<VulkanCoreInterface> _core;
		VkFence _dummyfence = nullptr;
		VkRenderPass RenderPass = nullptr;
		bool _vsync;
		bool _imgui;
		bool _clearswapchain;
		int _width{};
		int _height{};
		VkSurfaceCapabilitiesKHR _cabilities{};
		std::vector<VkSurfaceFormatKHR> _surfaceFormats;
		std::vector<VkPresentModeKHR> _presentationModes;
		std::vector<VkFramebuffer> _framebuffers;
		ref<CommandPool> _pool;
		std::vector<VkCommandBuffer> _cmd;
		ref<Semaphore> _presentlock;
		ref<Fence> _poollock;
		VkDescriptorPool _imguipool = nullptr;
	};

}
