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
#include "Utility/CppUtility.hpp"

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

		/// <summary>
		/// Creates swapchain for window and initalizes ImGui
		/// </summary>
		void EGX_API AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui);

		std::vector<Device> EGX_API EnumerateDevices();

		void EGX_API EstablishDevice(const Device& Device, bool UsingRenderDOC);

		inline ref<VulkanCoreInterface> GetCoreInterface() noexcept { return CoreInterface; }

		static EngineCore* CreateDefaultEngine(const std::string& title, uint32_t width, uint32_t height, bool VSync, bool SetupImGui, bool UsingRenderDOC, uint32_t MaxFramesInFlight = 2) {
			ref<PlatformWindow> window = new PlatformWindow(title, width, height);
			EngineCore* core = new EngineCore();
			core->_window = window;
			auto deviceList = core->EnumerateDevices();
			if (deviceList.size() == 0) {
				delete core;
				return nullptr;
			}
			auto& device = deviceList[0];
			int score = 0;
			for (auto& d : deviceList) {
				int s = d.IsDedicated ? 1000 : 0;
				s += d.VideoRam > device.VideoRam ? 1000 : 0;
				if (s > score)
					device = d;
			}
			core->EstablishDevice(device, UsingRenderDOC);
			core->AssociateWindow(window(), MaxFramesInFlight, VSync, SetupImGui);
			return core;
		}

		EGX_API ut::Logger* GetEngineLogger();

		// Default Window created by CreateDefaultEngine()
		inline ref<PlatformWindow> GetDefaultWindow() { return _window; }
		inline void WaitIdle() const { vkDeviceWaitIdle(CoreInterface->Device); }

		inline void SetImGuiContext() const {
			ImGui::SetCurrentContext(GetContext());
		}

	private:
		EGX_API ImGuiContext* GetContext() const;

	public:
		VulkanSwapchain* Swapchain = nullptr;
		ref<VulkanCoreInterface> CoreInterface;
	private:
		ref<PlatformWindow> _window;
	};

}
