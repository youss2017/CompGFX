#pragma once
#include "PlatformWindow.hpp"
#include <memory/egximage.hpp>
#include <pipeline/Sampler.hpp>
#include "swapchain.hpp"

namespace egx {

	class BitmapScene;

	class BitmapWindow : public PlatformWindow {
	public:
		BitmapWindow() = default;
		BitmapWindow(const DeviceCtx& ctx, std::string title, int width, int height);
		~BitmapWindow();
		void SwapBuffers();

		std::vector<uint32_t>& GetBackBuffer() { return m_BackBuffer; }

	private:
		BitmapScene* m_Scene;
		ISwapchainController m_Swapchain;
		std::vector<uint32_t> m_BackBuffer;
	};

}