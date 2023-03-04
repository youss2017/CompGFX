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
		EGX_API EngineCore(EngineCoreDebugFeatures debugFeatures, bool UsingBufferReference);
		EGX_API ~EngineCore();
		EGX_API EngineCore(EngineCore& cp) = delete;
		EGX_API EngineCore(EngineCore&& cp) = delete;
		EGX_API EngineCore& operator=(EngineCore&& cp) = delete;

		EGX_API EngineCore(const EngineCore& copy) = delete;
		EGX_API EngineCore(const EngineCore&& move) = delete;

		/// <summary>
		/// Creates swapchain for window and initalizes ImGui
		/// </summary>
		/// <returns>Swapchain Object</returns>
		inline VulkanSwapchain* AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui)
		{
			_AssociateWindow(Window, MaxFramesInFlight, VSync, SetupImGui);
			ImGui::SetCurrentContext(GetContext());
			return _Swapchain;
		}

		EGX_API std::vector<Device> EnumerateDevices();

		EGX_API ref<VulkanCoreInterface> EstablishDevice(const Device& Device, const VkPhysicalDeviceFeatures2& features = {});

		EGX_API cpp::Logger* GetEngineLogger();

		inline void WaitIdle() const { vkDeviceWaitIdle(_CoreInterface->Device); }

	private:
		const bool UsingBufferReference;
		EGX_API ImGuiContext* GetContext() const;
		void EGX_API _AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui);

	public:
	private:
		VkDebugReportCallbackEXT _DebugCallbackHandle;
		ref<VulkanCoreInterface> _CoreInterface;
		VulkanSwapchain* _Swapchain = nullptr;
	};

}
