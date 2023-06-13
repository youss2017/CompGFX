#include "DearImGuiController.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <core/ScopedCommandBuffer.hpp>

using namespace std;
using namespace egx;

egx::DearImGuiController::DearImGuiController(const DeviceCtx& ctx, GLFWwindow* window, vk::RenderPass renderpass)
{
	m_Data = make_shared<DearImGuiController::DataWrapper>();
	m_Data->m_Ctx = ctx;
	m_Data->m_Pool = CreateImGuiDescriptorPool();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;// | ImGuiConfigFlags_ViewportsEnable;
	io.ConfigDockingWithShift = true;
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = ctx->ICDState->Instance();
	init_info.PhysicalDevice = ctx->PhysicalDeviceQuery.PhysicalDevice;
	init_info.Device = ctx->Device;
	init_info.QueueFamily = ctx->GraphicsQueueFamilyIndex;
	init_info.Queue = ctx->Queue;
	init_info.PipelineCache = NULL;
	init_info.DescriptorPool = m_Data->m_Pool;
	init_info.Allocator = NULL;
	init_info.MinImageCount = ctx->FramesInFlight;
	init_info.ImageCount = ctx->FramesInFlight;
	init_info.CheckVkResultFn = NULL;
	ImGui_ImplVulkan_Init(&init_info, renderpass);

	m_Data->m_ImGuiContext = ImGui::GetCurrentContext();

	// Upload Fonts
	ScopedCommandBuffer cmd(ctx);
	ImGui_ImplVulkan_CreateFontsTexture(cmd.Get());
	cmd.RunNow();
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

ImGuiContext* egx::DearImGuiController::GetContext()
{
	return m_Data->m_ImGuiContext;
}

void egx::DearImGuiController::BeginFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void egx::DearImGuiController::EndFrame(vk::CommandBuffer cmd)
{
	ImGui::Render();
	auto draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
	auto& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

vk::DescriptorPool egx::DearImGuiController::CreateImGuiDescriptorPool(uint32_t maxSets)
{
	std::vector<vk::DescriptorPoolSize> poolSizes = {
		 { vk::DescriptorType::eSampler, 1000 },
		 { vk::DescriptorType::eCombinedImageSampler, 1000 },
		 { vk::DescriptorType::eSampledImage, 1000 },
		 { vk::DescriptorType::eStorageImage, 1000 },
		 { vk::DescriptorType::eUniformTexelBuffer, 1000 },
		 { vk::DescriptorType::eStorageTexelBuffer, 1000 },
		 { vk::DescriptorType::eUniformBuffer, 1000 },
		 { vk::DescriptorType::eStorageBuffer, 1000 },
		 { vk::DescriptorType::eUniformBufferDynamic, 1000 },
		 { vk::DescriptorType::eStorageBufferDynamic, 1000 },
		 { vk::DescriptorType::eInputAttachment, 1000 }
	};

	vk::DescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.maxSets = maxSets;
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCreateInfo.pPoolSizes = poolSizes.data();

	return m_Data->m_Ctx->Device.createDescriptorPool(poolCreateInfo);
}


DearImGuiController::DataWrapper::~DataWrapper()
{
	if (m_Ctx && m_ImGuiContext) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext(m_ImGuiContext);
		m_Ctx->Device.destroyDescriptorPool(m_Pool);
	}
}
