#pragma once
#include <core/egx.hpp>
#include <imgui/imgui.h>
#include <window/PlatformWindow.hpp>

namespace egx {

	class DearImGuiController {
	public:
		DearImGuiController() = default;
		DearImGuiController(const DeviceCtx& ctx, GLFWwindow* window, vk::RenderPass renderpass);
		ImGuiContext* GetContext();
		void BeginFrame();
		void EndFrame(vk::CommandBuffer cmd);

	private:
		vk::DescriptorPool CreateImGuiDescriptorPool(uint32_t maxSets = 1000);

	private:
		struct DataWrapper {
			DeviceCtx m_Ctx = nullptr;
			ImGuiContext* m_ImGuiContext = nullptr;
			vk::DescriptorPool m_Pool = nullptr;
			~DataWrapper();
		};
		std::shared_ptr<DataWrapper> m_Data;
	};

}
