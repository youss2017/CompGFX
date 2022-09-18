#pragma once
#include "egxcommon.hpp"
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include "../window/PlatformWindow.hpp"
#include <string_view>
#include "memory/egxmemory.hpp"

namespace egx {
	
	class egximguiwrapper {

	public:
		EGX_API static ref<egximguiwrapper> FactoryCreate(
			ref<VulkanCoreInterface> CoreInterface,
			GLFWwindow* GlfwWindowPtr,
			VkRenderPass RenderPass,
			uint32_t SwapchainImageCount);

		EGX_API egximguiwrapper(egximguiwrapper&) = delete;
		EGX_API egximguiwrapper(egximguiwrapper&& move) noexcept;
		EGX_API egximguiwrapper& operator=(egximguiwrapper& move) noexcept;
		EGX_API ~egximguiwrapper();

		EGX_API void BeginFrame() const;
		EGX_API void EndFrame(VkCommandBuffer cmd) const;

		EGX_API ImFont* LoadFont(std::string_view path, float size);
		EGX_API ImGuiContext* GetContext();

		// Image Layout must be VK_SHADER_LAYOUT_READ_ONLY_OPTIMIAL
		EGX_API ImTextureID CreateImGuiTexture(const ref<egx::Image>& image, uint32_t ViewId, VkSampler Sampler);

	protected:
		EGX_API egximguiwrapper(ref<VulkanCoreInterface> CoreInterface, VkDescriptorPool Pool) :
			_coreinterface(CoreInterface), _pool(Pool)
		{}

	protected:
		ref<VulkanCoreInterface> _coreinterface;
		VkDescriptorPool _pool;

	};

}
