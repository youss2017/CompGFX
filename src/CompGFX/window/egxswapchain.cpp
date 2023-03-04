#include "egxswapchain.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <algorithm>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <Utility/CppUtility.hpp>
#include "../cmd/cmd.hpp"

static const char* builtin_vertex_shader = R"(
	#version 450

	const vec4 verts[6] =
	{
		vec4(-1.0, -1.0, 0.0, 0.0),
		vec4(-1.0, 1.0, 0.0, 1.0),
		vec4(1.0, -1.0, 1.0, 0.0),
		vec4(-1.0, 1.0, 0.0, 1.0),
		vec4(1.0, 1.0, 1.0, 1.0),
		vec4(1.0, -1.0, 1.0, 0.0)
	};

	layout (location = 0) out vec2 UV;

	void main() {
		vec4 v = verts[gl_VertexIndex];
		UV = v.zw;
		gl_Position = vec4(v.xy, 0.0, 1.0);
	}
)";

static const char* builtin_fragment_shader = R"(
	#version 450

	layout (binding = 0) uniform sampler2D BlitImage; 
	layout (location = 0) in vec2 UV;
	layout (location = 0) out vec4 FragColor;

	void main()
	{
		FragColor = texture(BlitImage, UV);
	}
)";


static const uint8_t builtin_vs_bytecode[] = {
	3, 2, 35, 7, 0, 0, 1, 0, 10, 0, 13, 0, 47, 0, 0, 0, 0, 0, 0, 0, 17, 0, 2, 0, 1, 0, 0, 0, 11, 0, 6, 0, 1, 0, 0, 0, 71, 76, 83, 76, 46, 115, 116, 100, 46, 52, 53, 48, 0, 0, 0, 0, 14, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 15, 0, 8, 0, 0, 0, 0, 0, 4, 0, 0, 0, 109, 97, 105, 110, 0, 0, 0, 0, 23, 0, 0, 0, 31, 0, 0, 0, 38, 0, 0, 0, 3, 0, 3, 0, 2, 0, 0, 0, 194, 1, 0, 0, 4, 0, 10, 0, 71, 76, 95, 71, 79, 79, 71, 76, 69, 95, 99, 112, 112, 95, 115, 116, 121, 108, 101, 95, 108, 105, 110, 101, 95, 100, 105, 114, 101, 99, 116, 105, 118, 101, 0, 0, 4, 0, 8, 0, 71, 76, 95, 71, 79, 79, 71, 76, 69, 95, 105, 110, 99, 108, 117, 100, 101, 95, 100, 105, 114, 101, 99, 116, 105, 118, 101, 0, 5, 0, 4, 0, 4, 0, 0, 0, 109, 97, 105, 110, 0, 0, 0, 0, 5, 0, 3, 0, 9, 0, 0, 0, 118, 0, 0, 0, 5, 0, 6, 0, 23, 0, 0, 0, 103, 108, 95, 86, 101, 114, 116, 101, 120, 73, 110, 100, 101, 120, 0, 0, 5, 0, 5, 0, 26, 0, 0, 0, 105, 110, 100, 101, 120, 97, 98, 108, 101, 0, 0, 0, 5, 0, 3, 0, 31, 0, 0, 0, 85, 86, 0, 0, 5, 0, 6, 0, 36, 0, 0, 0, 103, 108, 95, 80, 101, 114, 86, 101, 114, 116, 101, 120, 0, 0, 0, 0, 6, 0, 6, 0, 36, 0, 0, 0, 0, 0, 0, 0, 103, 108, 95, 80, 111, 115, 105, 116, 105, 111, 110, 0, 6, 0, 7, 0, 36, 0, 0, 0, 1, 0, 0, 0, 103, 108, 95, 80, 111, 105, 110, 116, 83, 105, 122, 101, 0, 0, 0, 0, 6, 0, 7, 0, 36, 0, 0, 0, 2, 0, 0, 0, 103, 108, 95, 67, 108, 105, 112, 68, 105, 115, 116, 97, 110, 99, 101, 0, 6, 0, 7, 0, 36, 0, 0, 0, 3, 0, 0, 0, 103, 108, 95, 67, 117, 108, 108, 68, 105, 115, 116, 97, 110, 99, 101, 0, 5, 0, 3, 0, 38, 0, 0, 0, 0, 0, 0, 0, 71, 0, 4, 0, 23, 0, 0, 0, 11, 0, 0, 0, 42, 0, 0, 0, 71, 0, 4, 0, 31, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 72, 0, 5, 0, 36, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 72, 0, 5, 0, 36, 0, 0, 0, 1, 0, 0, 0, 11, 0, 0, 0, 1, 0, 0, 0, 72, 0, 5, 0, 36, 0, 0, 0, 2, 0, 0, 0, 11, 0, 0, 0, 3, 0, 0, 0, 72, 0, 5, 0, 36, 0, 0, 0, 3, 0, 0, 0, 11, 0, 0, 0, 4, 0, 0, 0, 71, 0, 3, 0, 36, 0, 0, 0, 2, 0, 0, 0, 19, 0, 2, 0, 2, 0, 0, 0, 33, 0, 3, 0, 3, 0, 0, 0, 2, 0, 0, 0, 22, 0, 3, 0, 6, 0, 0, 0, 32, 0, 0, 0, 23, 0, 4, 0, 7, 0, 0, 0, 6, 0, 0, 0, 4, 0, 0, 0, 32, 0, 4, 0, 8, 0, 0, 0, 7, 0, 0, 0, 7, 0, 0, 0, 21, 0, 4, 0, 10, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 43, 0, 4, 0, 10, 0, 0, 0, 11, 0, 0, 0, 6, 0, 0, 0, 28, 0, 4, 0, 12, 0, 0, 0, 7, 0, 0, 0, 11, 0, 0, 0, 43, 0, 4, 0, 6, 0, 0, 0, 13, 0, 0, 0, 0, 0, 128, 191, 43, 0, 4, 0, 6, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 44, 0, 7, 0, 7, 0, 0, 0, 15, 0, 0, 0, 13, 0, 0, 0, 13, 0, 0, 0, 14, 0, 0, 0, 14, 0, 0, 0, 43, 0, 4, 0, 6, 0, 0, 0, 16, 0, 0, 0, 0, 0, 128, 63, 44, 0, 7, 0, 7, 0, 0, 0, 17, 0, 0, 0, 13, 0, 0, 0, 16, 0, 0, 0, 14, 0, 0, 0, 16, 0, 0, 0, 44, 0, 7, 0, 7, 0, 0, 0, 18, 0, 0, 0, 16, 0, 0, 0, 13, 0, 0, 0, 16, 0, 0, 0, 14, 0, 0, 0, 44, 0, 7, 0, 7, 0, 0, 0, 19, 0, 0, 0, 16, 0, 0, 0, 16, 0, 0, 0, 16, 0, 0, 0, 16, 0, 0, 0, 44, 0, 9, 0, 12, 0, 0, 0, 20, 0, 0, 0, 15, 0, 0, 0, 17, 0, 0, 0, 18, 0, 0, 0, 17, 0, 0, 0, 19, 0, 0, 0, 18, 0, 0, 0, 21, 0, 4, 0, 21, 0, 0, 0, 32, 0, 0, 0, 1, 0, 0, 0, 32, 0, 4, 0, 22, 0, 0, 0, 1, 0, 0, 0, 21, 0, 0, 0, 59, 0, 4, 0, 22, 0, 0, 0, 23, 0, 0, 0, 1, 0, 0, 0, 32, 0, 4, 0, 25, 0, 0, 0, 7, 0, 0, 0, 12, 0, 0, 0, 23, 0, 4, 0, 29, 0, 0, 0, 6, 0, 0, 0, 2, 0, 0, 0, 32, 0, 4, 0, 30, 0, 0, 0, 3, 0, 0, 0, 29, 0, 0, 0, 59, 0, 4, 0, 30, 0, 0, 0, 31, 0, 0, 0, 3, 0, 0, 0, 43, 0, 4, 0, 10, 0, 0, 0, 34, 0, 0, 0, 1, 0, 0, 0, 28, 0, 4, 0, 35, 0, 0, 0, 6, 0, 0, 0, 34, 0, 0, 0, 30, 0, 6, 0, 36, 0, 0, 0, 7, 0, 0, 0, 6, 0, 0, 0, 35, 0, 0, 0, 35, 0, 0, 0, 32, 0, 4, 0, 37, 0, 0, 0, 3, 0, 0, 0, 36, 0, 0, 0, 59, 0, 4, 0, 37, 0, 0, 0, 38, 0, 0, 0, 3, 0, 0, 0, 43, 0, 4, 0, 21, 0, 0, 0, 39, 0, 0, 0, 0, 0, 0, 0, 32, 0, 4, 0, 45, 0, 0, 0, 3, 0, 0, 0, 7, 0, 0, 0, 54, 0, 5, 0, 2, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 248, 0, 2, 0, 5, 0, 0, 0, 59, 0, 4, 0, 8, 0, 0, 0, 9, 0, 0, 0, 7, 0, 0, 0, 59, 0, 4, 0, 25, 0, 0, 0, 26, 0, 0, 0, 7, 0, 0, 0, 61, 0, 4, 0, 21, 0, 0, 0, 24, 0, 0, 0, 23, 0, 0, 0, 62, 0, 3, 0, 26, 0, 0, 0, 20, 0, 0, 0, 65, 0, 5, 0, 8, 0, 0, 0, 27, 0, 0, 0, 26, 0, 0, 0, 24, 0, 0, 0, 61, 0, 4, 0, 7, 0, 0, 0, 28, 0, 0, 0, 27, 0, 0, 0, 62, 0, 3, 0, 9, 0, 0, 0, 28, 0, 0, 0, 61, 0, 4, 0, 7, 0, 0, 0, 32, 0, 0, 0, 9, 0, 0, 0, 79, 0, 7, 0, 29, 0, 0, 0, 33, 0, 0, 0, 32, 0, 0, 0, 32, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 62, 0, 3, 0, 31, 0, 0, 0, 33, 0, 0, 0, 61, 0, 4, 0, 7, 0, 0, 0, 40, 0, 0, 0, 9, 0, 0, 0, 79, 0, 7, 0, 29, 0, 0, 0, 41, 0, 0, 0, 40, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 81, 0, 5, 0, 6, 0, 0, 0, 42, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 0, 81, 0, 5, 0, 6, 0, 0, 0, 43, 0, 0, 0, 41, 0, 0, 0, 1, 0, 0, 0, 80, 0, 7, 0, 7, 0, 0, 0, 44, 0, 0, 0, 42, 0, 0, 0, 43, 0, 0, 0, 14, 0, 0, 0, 16, 0, 0, 0, 65, 0, 5, 0, 45, 0, 0, 0, 46, 0, 0, 0, 38, 0, 0, 0, 39, 0, 0, 0, 62, 0, 3, 0, 46, 0, 0, 0, 44, 0, 0, 0, 253, 0, 1, 0, 56, 0, 1, 0
};

static const uint8_t builtin_fs_bytecode[] = {
	3, 2, 35, 7, 0, 0, 1, 0, 10, 0, 13, 0, 20, 0, 0, 0, 0, 0, 0, 0, 17, 0, 2, 0, 1, 0, 0, 0, 11, 0, 6, 0, 1, 0, 0, 0, 71, 76, 83, 76, 46, 115, 116, 100, 46, 52, 53, 48, 0, 0, 0, 0, 14, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 15, 0, 7, 0, 4, 0, 0, 0, 4, 0, 0, 0, 109, 97, 105, 110, 0, 0, 0, 0, 9, 0, 0, 0, 17, 0, 0, 0, 16, 0, 3, 0, 4, 0, 0, 0, 7, 0, 0, 0, 3, 0, 3, 0, 2, 0, 0, 0, 194, 1, 0, 0, 4, 0, 10, 0, 71, 76, 95, 71, 79, 79, 71, 76, 69, 95, 99, 112, 112, 95, 115, 116, 121, 108, 101, 95, 108, 105, 110, 101, 95, 100, 105, 114, 101, 99, 116, 105, 118, 101, 0, 0, 4, 0, 8, 0, 71, 76, 95, 71, 79, 79, 71, 76, 69, 95, 105, 110, 99, 108, 117, 100, 101, 95, 100, 105, 114, 101, 99, 116, 105, 118, 101, 0, 5, 0, 4, 0, 4, 0, 0, 0, 109, 97, 105, 110, 0, 0, 0, 0, 5, 0, 5, 0, 9, 0, 0, 0, 70, 114, 97, 103, 67, 111, 108, 111, 114, 0, 0, 0, 5, 0, 5, 0, 13, 0, 0, 0, 66, 108, 105, 116, 73, 109, 97, 103, 101, 0, 0, 0, 5, 0, 3, 0, 17, 0, 0, 0, 85, 86, 0, 0, 71, 0, 4, 0, 9, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 71, 0, 4, 0, 13, 0, 0, 0, 34, 0, 0, 0, 0, 0, 0, 0, 71, 0, 4, 0, 13, 0, 0, 0, 33, 0, 0, 0, 0, 0, 0, 0, 71, 0, 4, 0, 17, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 19, 0, 2, 0, 2, 0, 0, 0, 33, 0, 3, 0, 3, 0, 0, 0, 2, 0, 0, 0, 22, 0, 3, 0, 6, 0, 0, 0, 32, 0, 0, 0, 23, 0, 4, 0, 7, 0, 0, 0, 6, 0, 0, 0, 4, 0, 0, 0, 32, 0, 4, 0, 8, 0, 0, 0, 3, 0, 0, 0, 7, 0, 0, 0, 59, 0, 4, 0, 8, 0, 0, 0, 9, 0, 0, 0, 3, 0, 0, 0, 25, 0, 9, 0, 10, 0, 0, 0, 6, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 27, 0, 3, 0, 11, 0, 0, 0, 10, 0, 0, 0, 32, 0, 4, 0, 12, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 59, 0, 4, 0, 12, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 23, 0, 4, 0, 15, 0, 0, 0, 6, 0, 0, 0, 2, 0, 0, 0, 32, 0, 4, 0, 16, 0, 0, 0, 1, 0, 0, 0, 15, 0, 0, 0, 59, 0, 4, 0, 16, 0, 0, 0, 17, 0, 0, 0, 1, 0, 0, 0, 54, 0, 5, 0, 2, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 248, 0, 2, 0, 5, 0, 0, 0, 61, 0, 4, 0, 11, 0, 0, 0, 14, 0, 0, 0, 13, 0, 0, 0, 61, 0, 4, 0, 15, 0, 0, 0, 18, 0, 0, 0, 17, 0, 0, 0, 87, 0, 5, 0, 7, 0, 0, 0, 19, 0, 0, 0, 14, 0, 0, 0, 18, 0, 0, 0, 62, 0, 3, 0, 9, 0, 0, 0, 19, 0, 0, 0, 253, 0, 1, 0, 56, 0, 1, 0
};

using namespace egx;

egx::VulkanSwapchain::VulkanSwapchain(ref<VulkanCoreInterface>& CoreInterface, void* GlfwWindowPtr, bool VSync, bool SetupImGui)
	: GlfwWindowPtr(GlfwWindowPtr), _vsync(VSync), _imgui(SetupImGui), ISynchronization(CoreInterface, "VulkanSwapchain")
{
	VkBool32 Supported{};
	glfwCreateWindowSurface(_CoreInterface->Instance, (GLFWwindow*)GlfwWindowPtr, nullptr, &Surface);
	vkGetPhysicalDeviceSurfaceSupportKHR(CoreInterface->PhysicalDevice.Id, CoreInterface->GraphicsQueueFamilyIndex, Surface, &Supported);
	if (!Supported)
	{
		vkDestroySurfaceKHR(_CoreInterface->Instance, Surface, nullptr);
		throw std::runtime_error("The provided window does not support swapchain presentation. Look up vkGetPhysicalDeviceSurfaceSupportKHR()");
	}
	uint32_t surfaceCount, presentationModeCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_CoreInterface->PhysicalDevice.Id, Surface, &surfaceCount, nullptr);
	_surfaceFormats.resize(surfaceCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(_CoreInterface->PhysicalDevice.Id, Surface, &surfaceCount, _surfaceFormats.data());
	vkGetPhysicalDeviceSurfacePresentModesKHR(_CoreInterface->PhysicalDevice.Id, Surface, &presentationModeCount, nullptr);
	_presentationModes.resize(presentationModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(_CoreInterface->PhysicalDevice.Id, Surface, &presentationModeCount, _presentationModes.data());

	glfwGetFramebufferSize((GLFWwindow*)GlfwWindowPtr, &_width, &_height);

	_pool = CreateCommandPool(_CoreInterface, true, false, true, false);
	_cmd = _pool->AllocateBufferFrameFlightMode(true);
	GetOrCreateCompletionFence();

	_acquire_lock = Semaphore::FactoryCreate(CoreInterface, "AcquireNextImageLock");

	_descriptor_set_pool = SetPool::FactoryCreate(CoreInterface);
	SetPoolRequirementsInfo req;
	req.Type[VK_DESCRIPTOR_TYPE_SAMPLER] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC] = 1000;
	req.Type[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT] = 1000;
	_descriptor_set_pool->AdjustForRequirements(req);
	_descriptor_set_pool->IncrementSetCount(1000);

	{
		VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = sizeof(builtin_vs_bytecode);
		createInfo.pCode = (uint32_t*)builtin_vs_bytecode;
		vkCreateShaderModule(CoreInterface->Device, &createInfo, nullptr, &_builtin_vertex_module);
	}
	{
		VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = sizeof(builtin_fs_bytecode);
		createInfo.pCode = (uint32_t*)builtin_fs_bytecode;
		vkCreateShaderModule(CoreInterface->Device, &createInfo, nullptr, &_builtin_fragment_module);
	}

	CreateRenderPass();
	CreateSwapchain(_width, _height);
	CreatePipelineObjects();
	CreatePipeline();

	if (_imgui)
	{
#if defined(VK_NO_PROTOTYPES)
		struct ImGUI_LoadFunctions_UserData
		{
			VkInstance instance;
			VkDevice device;
		};
		ImGUI_LoadFunctions_UserData ImGui_Load_UserData;
		ImGui_Load_UserData.instance = CoreInterface->Instance;
		ImGui_Load_UserData.device = CoreInterface->Device;
		ImGui_ImplVulkan_LoadFunctions(
			[](const char* function_name, void* user_data) throw()->PFN_vkVoidFunction {
				ImGUI_LoadFunctions_UserData* pUserData = (ImGUI_LoadFunctions_UserData*)user_data;
				PFN_vkVoidFunction proc = (PFN_vkVoidFunction)vkGetDeviceProcAddr(pUserData->device, function_name);
				if (!proc)
					return vkGetInstanceProcAddr(pUserData->instance, function_name);
				else
					return proc;
			},
			&ImGui_Load_UserData);
#endif
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
		// io.ConfigDockingWithShift = true;

		VkQueue queue;
		vkGetDeviceQueue(CoreInterface->Device, CoreInterface->GraphicsQueueFamilyIndex, 0, &queue);
		ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)this->GlfwWindowPtr, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = CoreInterface->Instance;
		init_info.PhysicalDevice = CoreInterface->PhysicalDevice.Id;
		init_info.Device = CoreInterface->Device;
		init_info.QueueFamily = CoreInterface->GraphicsQueueFamilyIndex;
		init_info.Queue = queue;
		init_info.PipelineCache = NULL;
		init_info.DescriptorPool = _descriptor_set_pool->GetSetPool();
		init_info.Allocator = NULL;
		init_info.MinImageCount = this->_CoreInterface->MaxFramesInFlight;
		init_info.ImageCount = this->_CoreInterface->MaxFramesInFlight;
		init_info.CheckVkResultFn = NULL;
		ImGui_ImplVulkan_Init(&init_info, RenderPass);
		// ImFont* fftt = io.Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCodePL-SemiBold.ttf", 14.5f);

		// Upload Fonts
		{
			// Use any command queue
			VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			VkCommandPool command_pool;
			vkCreateCommandPool(CoreInterface->Device, &createInfo, nullptr, &command_pool);
			VkCommandBuffer command_buffer;
			VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			allocInfo.commandPool = command_pool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			vkAllocateCommandBuffers(CoreInterface->Device, &allocInfo, &command_buffer);

			vkResetCommandPool(CoreInterface->Device, command_pool, 0);
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(command_buffer, &begin_info);

			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &command_buffer;
			vkEndCommandBuffer(command_buffer);
			vkQueueSubmit(queue, 1, &end_info, NULL);

			vkQueueWaitIdle(queue);
			ImGui_ImplVulkan_DestroyFontUploadObjects();
			vkDestroyCommandPool(CoreInterface->Device, command_pool, nullptr);
		}
	}

}

egx::VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other) noexcept
	: ISynchronization(std::move(other))
{
	memcpy(this, &other, sizeof(VulkanSwapchain));
	memset(&other, 0, sizeof(VulkanCoreInterface));
}

egx::VulkanSwapchain& egx::VulkanSwapchain::operator=(VulkanSwapchain& move) noexcept
{
	if (this == &move) return *this;
	this->~VulkanSwapchain();
	memcpy(this, &move, sizeof(VulkanSwapchain));
	memset(&move, 0, sizeof(VulkanCoreInterface));
	return *this;
}

egx::VulkanSwapchain::~VulkanSwapchain()
{
	if (_imgui)
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	if (RenderPass)
		vkDestroyRenderPass(_CoreInterface->Device, RenderPass, nullptr);
	if (Swapchain)
		vkDestroySwapchainKHR(_CoreInterface->Device, Swapchain, nullptr);
	if (Surface)
		vkDestroySurfaceKHR(_CoreInterface->Instance, Surface, nullptr);
	for (auto view : Views)
		vkDestroyImageView(_CoreInterface->Device, view, nullptr);
	for (auto fbo : _framebuffers)
		vkDestroyFramebuffer(_CoreInterface->Device, fbo, nullptr);
	if (_blit_layout)
		vkDestroyPipelineLayout(_CoreInterface->Device, _blit_layout, nullptr);
	if (_descriptor_layout)
		vkDestroyDescriptorSetLayout(_CoreInterface->Device, _descriptor_layout, nullptr);
	if (_blit_gfx)
		vkDestroyPipeline(_CoreInterface->Device, _blit_gfx, nullptr);
	vkDestroyShaderModule(_CoreInterface->Device, _builtin_vertex_module, nullptr);
	vkDestroyShaderModule(_CoreInterface->Device, _builtin_fragment_module, nullptr);
	GetOrCreateCompletionFence()->SynchronizeAllFrames();
}

void egx::VulkanSwapchain::Acquire()
{
	if (_recreate_swapchain_flag) {
		int w, h;
		glfwGetFramebufferSize((GLFWwindow*)GlfwWindowPtr, &w, &h);
		if (w > 0 && h > 0 && (w != _width || h != _height)) {
			vkDeviceWaitIdle(_CoreInterface->Device);
			CreateSwapchain(_width, _height);
			CreatePipeline();
			_recreate_swapchain_flag = false;
			CommandBufferSingleUse cmd(_CoreInterface);
			for (uint32_t i = 0; i < Imgs.size(); i++) {
				VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
				barrier.srcAccessMask = VK_ACCESS_NONE;
				barrier.dstAccessMask = VK_ACCESS_NONE;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = Imgs[i];
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.layerCount = 1;
				barrier.subresourceRange.levelCount = 1;
				vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
			}
			cmd.Execute();
		}
	}
	VkResult result = vkAcquireNextImageKHR(_CoreInterface->Device,
		Swapchain,
		UINT64_MAX,
		_acquire_lock->GetSemaphore(),
		nullptr,
		&_image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		_recreate_swapchain_flag = true;
	}
	if (_imgui)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
}

uint32_t egx::VulkanSwapchain::PresentInit()
{
	SetExecuting();
	WaitAndReset();
	return _CoreInterface->CurrentFrame;
}

void egx::VulkanSwapchain::Present(const ref<Image>& image, uint32_t viewIndex)
{
	uint32_t frame = PresentInit();

	if (image->Image_ != _last_updated_image[frame])
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = _image_sampler->GetSampler();
		imageInfo.imageView = image->View(viewIndex);
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.dstSet = _descriptor_set[frame];
		write.dstBinding = 0;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(_CoreInterface->Device, 1, &write, 0, nullptr);
		_last_updated_image[frame] = image->Image_;
	}

	CommandBuffer::StartCommandBuffer(_cmd[frame], 0);
	BeginDebugLabelCmd(_CoreInterface, _cmd[frame], "Swapchain Present", 0.5f, 0.4f, 0.65f);

	VkClearValue clear{};
	VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	beginInfo.renderPass = RenderPass;
	beginInfo.framebuffer = _framebuffers[frame];
	beginInfo.renderArea = { {}, {(uint32_t)_width, (uint32_t)_height} };
	beginInfo.clearValueCount = 1;
	beginInfo.pClearValues = &clear;
	vkCmdBeginRenderPass(_cmd[frame], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(_cmd[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, _blit_gfx);
	vkCmdBindDescriptorSets(_cmd[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, _blit_layout, 0, 1, &_descriptor_set[frame], 0, nullptr);
	vkCmdDraw(_cmd[frame], 6, 1, 0, 0);
	if (_imgui)
	{
		ImGui::Render();
		auto draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, _cmd[frame]);
	}
	vkCmdEndRenderPass(_cmd[frame]);

	vkEndCommandBuffer(_cmd[frame]);

	EndDebugLabelCmd(_CoreInterface, _cmd[frame]);

	PresentCommon(frame, false);
}

void egx::VulkanSwapchain::Present()
{
	assert(_imgui && "To use default Present() you must have enabled imgui otherwise you are presenting nothing.");
	uint32_t frame = PresentInit();

	CommandBuffer::StartCommandBuffer(_cmd[frame], 0);
	VkClearValue clear{};
	VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	beginInfo.renderPass = RenderPass;
	beginInfo.framebuffer = _framebuffers[frame];
	beginInfo.renderArea = { {}, {(uint32_t)_width, (uint32_t)_height} };
	beginInfo.clearValueCount = 1;
	beginInfo.pClearValues = &clear;
	vkCmdBeginRenderPass(_cmd[frame], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	if (_imgui)
	{
		ImGui::Render();
		auto draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, _cmd[frame]);
	}
	vkCmdEndRenderPass(_cmd[frame]);
	vkEndCommandBuffer(_cmd[frame]);

	PresentCommon(frame, true);
}

void egx::VulkanSwapchain::PresentCommon(uint32_t frame, bool onlyimgui)
{
	if (_imgui)
	{
		auto& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			auto backup_context = ImGui::GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			ImGui::SetCurrentContext(backup_context);
		}
	}

	VkSemaphore blitLock = GetOrCreateCompletionSemaphore()->GetSemaphore();

	Submit(_cmd[frame]);

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &blitLock;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &Swapchain;
	presentInfo.pImageIndices = &_image_index;
	vkQueuePresentKHR(_CoreInterface->Queue, &presentInfo);

	_CoreInterface->CurrentFrame = (_CoreInterface->CurrentFrame + 1u) % _CoreInterface->MaxFramesInFlight;
	_CoreInterface->FrameCount++;
}

void egx::VulkanSwapchain::SetSyncInterval(bool VSync)
{
	_vsync = VSync;
	_recreate_swapchain_flag = true;
}

void egx::VulkanSwapchain::Resize(int width, int height)
{
	if (width == 0 || height == 0)
		return;
	_recreate_swapchain_flag = true;
}

void egx::VulkanSwapchain::Resize()
{
	int w, h;
	glfwGetFramebufferSize((GLFWwindow*)GlfwWindowPtr, &w, &h);
	if (w == 0 || h == 0)
		return;
	_width = w, _height = h;
	Resize(w, h);
}

VkSurfaceFormatKHR egx::VulkanSwapchain::GetSurfaceFormat()
{
	std::vector<VkFormat> desiredList = {
		{VK_FORMAT_B8G8R8A8_UNORM},
		{VK_FORMAT_R8G8B8A8_UNORM},
		{VK_FORMAT_B8G8R8A8_UINT},
		{VK_FORMAT_R8G8B8A8_UINT},
		{VK_FORMAT_B8G8R8A8_SINT},
		{VK_FORMAT_R8G8B8A8_SINT},
		{VK_FORMAT_B8G8R8_UNORM},
		{VK_FORMAT_R8G8B8_UNORM},
		{VK_FORMAT_B8G8R8A8_UINT},
		{VK_FORMAT_R8G8B8A8_UINT},
		{VK_FORMAT_B8G8R8_UINT},
		{VK_FORMAT_R8G8B8_UINT},
		{VK_FORMAT_B8G8R8_SINT},
		{VK_FORMAT_R8G8B8_SINT},
		{VK_FORMAT_R8G8B8_UNORM} };
	std::vector<VkFormat> supportedList;
	for (auto& surfaceFormat : _surfaceFormats)
	{
		supportedList.push_back(surfaceFormat.format);
	}
	for (int i = 0; i < desiredList.size(); i++)
	{
		auto temp = std::find(supportedList.begin(), supportedList.end(), desiredList[i]);
		if (temp != supportedList.end())
			return _surfaceFormats[temp - supportedList.begin()];
	}
	return _surfaceFormats[0];
}

VkPresentModeKHR egx::VulkanSwapchain::GetPresentMode()
{
	if (!_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_MAILBOX_KHR))
		return VK_PRESENT_MODE_MAILBOX_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR))
		return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_FIFO_KHR))
		return VK_PRESENT_MODE_FIFO_KHR;
	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

void egx::VulkanSwapchain::CreateSwapchain(int width, int height)
{
	vkDeviceWaitIdle(_CoreInterface->Device);
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_CoreInterface->PhysicalDevice.Id, Surface, &_capabilities);
	VkSurfaceFormatKHR surfaceFormat = GetSurfaceFormat();
	_width = width;
	_height = height;

	for (auto view : Views)
		vkDestroyImageView(_CoreInterface->Device, view, nullptr);
	for (auto fbo : _framebuffers)
		vkDestroyFramebuffer(_CoreInterface->Device, fbo, nullptr);

	VkExtent2D swapchainSize =
	{
		std::clamp<uint32_t>(width, _capabilities.minImageExtent.width, _capabilities.maxImageExtent.width),
		std::clamp<uint32_t>(height, _capabilities.minImageExtent.height, _capabilities.maxImageExtent.height)
	};
	_width = swapchainSize.width;
	_height = swapchainSize.height;

	_CoreInterface->MaxFramesInFlight = std::max<uint32_t>(_CoreInterface->MaxFramesInFlight, _capabilities.minImageCount);

	VkSwapchainKHR oldSwapchain = Swapchain;
	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface = Surface;
	createInfo.minImageCount = _CoreInterface->MaxFramesInFlight;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchainSize;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 1;
	createInfo.pQueueFamilyIndices = (uint32_t*)&_CoreInterface->GraphicsQueueFamilyIndex;
	createInfo.preTransform = _capabilities.currentTransform; // VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR /* VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR */;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = GetPresentMode();
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;
	if (vkCreateSwapchainKHR(_CoreInterface->Device, &createInfo, nullptr, &Swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("Swapchain creation failed.");
	}
	if (oldSwapchain)
		vkDestroySwapchainKHR(_CoreInterface->Device, oldSwapchain, nullptr);
	uint32_t count;
	vkGetSwapchainImagesKHR(_CoreInterface->Device, Swapchain, &count, nullptr);
	Imgs.resize(count);
	vkGetSwapchainImagesKHR(_CoreInterface->Device, Swapchain, &count, Imgs.data());
	Views.resize(count);
	_framebuffers.resize(Views.size());

	for (uint32_t i = 0; i < count; i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = Imgs[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = surfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		VkImageView view;
		vkCreateImageView(_CoreInterface->Device, &createInfo, nullptr, &view);
		Views[i] = view;
	}
	for (int i = 0; i < Views.size(); i++)
	{
		VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = RenderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &Views[i];
		framebufferCreateInfo.width = _width;
		framebufferCreateInfo.height = _height;
		framebufferCreateInfo.layers = 1;
		vkCreateFramebuffer(_CoreInterface->Device, &framebufferCreateInfo, nullptr, &_framebuffers[i]);
	}
}

void egx::VulkanSwapchain::CreateRenderPass()
{
	VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;

	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = GetSurfaceFormat().format;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	bool viewportsEnabled = _imgui;/* ? (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) : false;*/

	// Extracted from ImGui vulkan backend
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo passCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	passCreateInfo.attachmentCount = 1;
	passCreateInfo.pAttachments = &attachmentDescription;
	passCreateInfo.subpassCount = 1;
	passCreateInfo.pSubpasses = &subpass;
	passCreateInfo.dependencyCount = viewportsEnabled ? 1 : 0;
	passCreateInfo.pDependencies = viewportsEnabled ? &dependency : nullptr;
	vkCreateRenderPass(_CoreInterface->Device, &passCreateInfo, nullptr, &RenderPass);
}

void VulkanSwapchain::CreatePipelineObjects()
{
	_image_sampler = Sampler::FactoryCreate(_CoreInterface);
	{
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		VkDescriptorSetLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		createInfo.bindingCount = 1;
		createInfo.pBindings = &binding;
		vkCreateDescriptorSetLayout(_CoreInterface->Device, &createInfo, nullptr, &_descriptor_layout);
	}
	{
		VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocateInfo.descriptorPool = _descriptor_set_pool->GetSetPool();
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &_descriptor_layout;
		_descriptor_set.resize(_framebuffers.size());
		for (size_t i = 0; i < _descriptor_set.size(); i++)
			vkAllocateDescriptorSets(_CoreInterface->Device, &allocateInfo, &_descriptor_set[i]);
		_last_updated_image.resize(_framebuffers.size(), nullptr);
	}
	{
		VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		createInfo.setLayoutCount = 1;
		createInfo.pSetLayouts = &_descriptor_layout;
		vkCreatePipelineLayout(_CoreInterface->Device, &createInfo, nullptr, &_blit_layout);
	}
	//_builtin_vertex = Shader2::FactoryCreateEx(_CoreInterface, builtin_vertex_shader, VK_SHADER_STAGE_VERTEX_BIT, egx::BindingAttributes::Default, "_builting_vertex(swapchain)");
	//_builtin_fragment = Shader2::FactoryCreateEx(_CoreInterface, builtin_fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT, egx::BindingAttributes::Default, "_builting_fragment(swapchain)");
}

void VulkanSwapchain::CreatePipeline()
{
	if (_blit_gfx)
		vkDestroyPipeline(_CoreInterface->Device, _blit_gfx, nullptr);
	VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	createInfo.stageCount = 2;
	VkPipelineShaderStageCreateInfo stages[2]{};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = _builtin_vertex_module; //_builtin_vertex->ShaderModule;
	stages[0].pName = "main";

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = _builtin_fragment_module; //_builtin_fragment->ShaderModule;
	stages[1].pName = "main";
	createInfo.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	createInfo.pVertexInputState = &vertexInputState;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.pInputAssemblyState = &inputAssembly;

	VkPipelineTessellationStateCreateInfo tessellation{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	createInfo.pTessellationState = &tessellation;

	VkViewport viewport{};
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.width = float(_width);
	viewport.height = float(_height);
	VkRect2D scissor{};
	scissor.extent = { uint32_t(_width), uint32_t(_height) };
	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	createInfo.pViewportState = &viewportState;

	VkPipelineRasterizationStateCreateInfo rasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationState.lineWidth = 1.0f;
	createInfo.pRasterizationState = &rasterizationState;

	VkPipelineMultisampleStateCreateInfo multisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.pMultisampleState = &multisampleState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	createInfo.pDepthStencilState = &depthStencilState;

	VkPipelineColorBlendAttachmentState attachment{};
	attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &attachment;
	createInfo.pColorBlendState = &colorBlendState;

	createInfo.pDynamicState = nullptr;
	createInfo.layout = _blit_layout;
	createInfo.renderPass = RenderPass;
	VkResult result = vkCreateGraphicsPipelines(_CoreInterface->Device, nullptr, 1, &createInfo, nullptr, &_blit_gfx);
	if (result != VK_SUCCESS) {
		LOG(ERR, "Fatal! Could not create swapchain pipeline!");
		cpp::DebugBreak();
	}
}
