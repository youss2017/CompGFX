#pragma once
#include <core/egx.hpp>
#include <window/PlatformWindow.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>

namespace egx
{
	class ISwapchainController : public SynchronizationPrimitive
	{
	public:
		ISwapchainController() = default;
		ISwapchainController(const DeviceCtx& pCtx, PlatformWindow* pTargetWindow, vk::Format format = vk::Format::eUndefined);

		ISwapchainController& SetVSync(bool state)
		{
			m_Data->m_VSync = state;
			Invalidate(true);
			return *this;
		}

		ISwapchainController& SetResizeOnWindowResize(bool resizeOnWindowResize)
		{
			m_Data->m_ResizeOnWindowResize = resizeOnWindowResize;
			return *this;
		}

		ISwapchainController& SetupImGui()
		{
			m_Data->m_SetupImGui = true;
			return *this;
		}

		ISwapchainController& SetSurfaceFormat(vk::SurfaceFormatKHR surfaceFormat)
		{
			m_Data->m_SurfaceFormat = surfaceFormat;
			return *this;
		}

		ISwapchainController& SetSize(int width, int height)
		{
			m_Data->m_Width = width, m_Data->m_Height = height;
			return *this;
		}

		const std::vector<vk::SurfaceFormatKHR>& QuerySurfaceFormats() { return m_Data->m_SurfaceFormats; }
		ImGuiContext* GetContext() { return m_Data->m_Context; }

		std::vector<vk::Image> GetSwapchainBackBuffers()
		{
			return m_Data->m_Ctx->Device.getSwapchainImagesKHR(m_Data->m_Swapchain);
		}

		void Invalidate(bool blockQueue = true);

		void Resize(int width, int height, bool blockQueue = true);

		void AddResizeCallback(ICopyableCallback* callbackObject, void* pUserData) { m_Data->m_ResizeCallbacks.push_back({ callbackObject->_MakeHandle(), pUserData}); }

		int Width() const { return m_Data->m_Width; }
		int Height() const { return m_Data->m_Height; }
		std::vector<vk::Image> GetBackBuffers() const { return m_Data->m_Ctx->Device.getSwapchainImagesKHR(m_Data->m_Swapchain); }
		vk::Format GetFormat() const { return m_Data->m_Format; }

		/// @brief Acquires next frame from swapchain
		/// @return ImageReadySemaphore, you must wait on this semaphore before drawing onto swapchain IRenderTarget
		vk::Semaphore Acquire();
		uint32_t AcquireFullLock();
		inline void Present(vk::Semaphore semaphore) { Present({ semaphore }); }
		void Present(const std::vector<vk::Semaphore>& presentReadySemaphore);
		// Presents using PresentWaitSemaphore
		void Present() {
			Present(m_Data->m_PresentWait);
		}

	private:
		struct DataWrapper
		{
			DeviceCtx m_Ctx;
			PlatformWindow* m_Window;
			bool m_VSync = true;
			bool m_ResizeOnWindowResize = true;
			bool m_SetupImGui = false;
			int m_FramesInFlight = 1;
			int m_Width;
			int m_Height;
			vk::Fence m_AcquireFullLock;
			vk::Format m_Format;

			std::vector<vk::SurfaceFormatKHR> m_SurfaceFormats;
			vk::SurfaceFormatKHR m_SurfaceFormat = vk::Format::eB8G8R8A8Unorm;

			uint32_t m_CurrentBackBufferIndex = 0;
			std::vector<vk::Semaphore> m_ImageReady;
			// The first semaphore is the current ImageReady semaphore
			std::vector<vk::Semaphore> m_PresentWait;
			vk::SurfaceKHR m_Surface;
			vk::SwapchainKHR m_Swapchain;
			ImGuiContext* m_Context = nullptr;
			std::vector<std::pair<std::unique_ptr<ICopyableCallback>, void*>> m_ResizeCallbacks;

			~DataWrapper();
		};

	private:
		std::shared_ptr<DataWrapper> m_Data;
	};
}
