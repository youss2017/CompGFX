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

	enum class EngineCoreDebugFeatures
	{
		GPUAssisted,
		DebugPrintfEXT
	};

	class EngineCore {
	public:
		// debugFeatures only available in debug mode
		EGX_API EngineCore(EngineCoreDebugFeatures debugFeatures, bool UsingRenderDOC);
		EGX_API ~EngineCore();
		EGX_API EngineCore(EngineCore& cp) = delete;
		EGX_API EngineCore(EngineCore&& cp) = delete;
		EGX_API EngineCore& operator=(EngineCore&& cp) = delete;

		EGX_API EngineCore(const EngineCore& copy) = delete;
		EGX_API EngineCore(const EngineCore&& move) = delete;

		/// <summary>
		/// Creates swapchain for window and initalizes ImGui
		/// </summary>
		inline void AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui)
		{
			_AssociateWindow(Window, MaxFramesInFlight, VSync, SetupImGui);
			ImGui::SetCurrentContext(GetContext());
		}

		std::vector<Device> EGX_API EnumerateDevices();

		void EGX_API EstablishDevice(const Device& Device);

		inline ref<VulkanCoreInterface> GetCoreInterface() noexcept { return CoreInterface; }
				EGX_API cpp::Logger* GetEngineLogger();

		inline void WaitIdle() const { vkDeviceWaitIdle(CoreInterface->Device); }

	private:
		const bool UsingRenderDOC;
		EGX_API ImGuiContext* GetContext() const;
		void EGX_API _AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui);

	public:
		VulkanSwapchain* Swapchain = nullptr;
		ref<VulkanCoreInterface> CoreInterface;
	private:
		VkDebugReportCallbackEXT _DebugCallbackHandle;
	};

}
