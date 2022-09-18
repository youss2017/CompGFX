#pragma once
#include "egxcommon.hpp"
#include "vulkinc.hpp"
#include "../window/PlatformWindow.hpp"
#include <string>
#include <vector>
#include "egxcommon.hpp"
#include "memory/vulkan_memory_allocator.hpp"
#include "../window/egxswapchain.hpp"
#include "egxutil.hpp"

namespace egx {

	class EngineCore {
	public:
		EGX_API EngineCore();
		EGX_API ~EngineCore();
		EGX_API EngineCore(EngineCore& cp) = delete;
		EGX_API EngineCore(EngineCore&& cp) = delete;
		EGX_API EngineCore& operator=(EngineCore&& cp) = delete;

		EGX_API EngineCore(const EngineCore& copy) = delete;
		EGX_API EngineCore(const EngineCore&& move) = delete;

		EGX_API void SetDebugOnErrorDialog(bool state) noexcept;

		/// <summary>
		/// Creates swapchain for window and initalizes ImGui
		/// </summary>
		void EGX_API AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui, bool ClearSwapchain);

		std::vector<Device> EGX_API EnumerateDevices();

		void EGX_API EstablishDevice(const Device& Device, bool UsingRenderDOC);

		inline ref<VulkanCoreInterface> GetCoreInterface() noexcept { return CoreInterface; }

		static EngineCore* CreateDefaultEngine(const std::string& title, uint32_t width, uint32_t height, bool VSync, bool SetupImGui, bool ClearSwapchain, bool UsingRenderDOC, uint32_t MaxFramesInFlight = 2) {
			ref<PlatformWindow> window = { new PlatformWindow(title, width, height) };
			EngineCore* core = new EngineCore();
			core->_window = window;
			core->EstablishDevice(core->EnumerateDevices()[0], UsingRenderDOC);
			core->AssociateWindow(window(), MaxFramesInFlight, VSync, SetupImGui, ClearSwapchain);
			return core;
		}

		// Default Window created by CreateDefaultEngine()
		inline ref<PlatformWindow> GetDefaultWindow() { return _window; }
		inline void WaitIdle() const { vkDeviceWaitIdle(CoreInterface->Device); }

	public:
		VulkanSwapchain* Swapchain;
		ref<VulkanCoreInterface> CoreInterface;
	private:
		ref<PlatformWindow> _window;
	};

}
