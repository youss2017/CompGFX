#include "EngineCore.hpp"
#include "GraphicsCardFeatureValidation.hpp"
#include <iostream>
#include <Utility/CppUtility.hpp>
#include <cassert>

static VkBool32 VKAPI_CALL ApiDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

EGX_API egx::EngineCore::EngineCore()
	: Swapchain(nullptr), CoreInterface(new VulkanCoreInterface())
{
	glfwInit();
	std::vector<const char*> layer_extensions;
	uint32_t extensions_count;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
	for (uint32_t i = 0; i < extensions_count; i++) layer_extensions.push_back(extensions[i]);
	std::vector<const char*> layers;
#ifdef _DEBUG
	layers.push_back("VK_LAYER_KHRONOS_validation");
	layer_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	VkApplicationInfo appinfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appinfo.pApplicationName = "Application";
	appinfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	appinfo.pEngineName = "Application";
	appinfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	appinfo.apiVersion = VK_API_VERSION_1_3;
	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appinfo;
	createInfo.enabledLayerCount = (uint32_t)layers.size();
	createInfo.ppEnabledLayerNames = layers.data();
	createInfo.enabledExtensionCount = (uint32_t)layer_extensions.size();
	createInfo.ppEnabledExtensionNames = layer_extensions.data();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#ifdef _DEBUG
	debugCreateInfo = {};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = ApiDebugCallback;
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif

	vkCreateInstance(&createInfo, NULL, &CoreInterface->Instance);
#if defined(VK_NO_PROTOTYPES)
	volkLoadInstance(instance);
#endif
#ifdef _DEBUG
	if (CreateDebugUtilsMessengerEXT(CoreInterface->Instance, &debugCreateInfo, nullptr, &CoreInterface->DebugMessenger) != VK_SUCCESS)
	{
		LOG(WARNING, "Failed to set up debug messenger!");
	}
#endif

#if defined(VK_NO_PROTOTYPES)
	volkLoadDevice(s_Context->defaultDevice);
#endif

}

EGX_API egx::EngineCore::~EngineCore()
{
	WaitIdle();
	if (Swapchain)
		delete Swapchain;
}

void EGX_API egx::EngineCore::AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui, bool ClearSwapchain)
{
	assert(Swapchain == nullptr);
	// [NOTE]: ImGui multi-viewport uses VK_FORMAT_B8G8R8A8_UNORM, if we use a different format
	// [NOTE]: there will be a mismatch of format between pipeline state objects and render pass
	CoreInterface->MaxFramesInFlight = MaxFramesInFlight;
	Swapchain = new VulkanSwapchain(
		CoreInterface,
		Window->GetWindow(),
		VSync,
		SetupImGui,
		ClearSwapchain);
}

std::vector<egx::Device> EGX_API egx::EngineCore::EnumerateDevices()
{
	std::vector<egx::Device> list;
	uint32_t Count = 0;
	vkEnumeratePhysicalDevices(CoreInterface->Instance, &Count, nullptr);
	std::vector<VkPhysicalDevice> Ids(Count);
	vkEnumeratePhysicalDevices(CoreInterface->Instance, &Count, Ids.data());

	for (auto device : Ids) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);
		egx::Device d{};
		d.IsDedicated = !((properties.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) |
			(properties.deviceType & VK_PHYSICAL_DEVICE_TYPE_CPU));
		d.Id = device;
		d.Properties = properties;
		d.VendorName = properties.deviceName;
		VkPhysicalDeviceMemoryProperties memProp;
		vkGetPhysicalDeviceMemoryProperties(device, &memProp);
		for (uint32_t j = 0; j < memProp.memoryHeapCount; j++) {
			if (memProp.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				d.VideoRam += memProp.memoryHeaps[j].size;
			}
			else {
				d.SharedSystemRam += memProp.memoryHeaps[j].size;
			}
		}
		list.push_back(d);
	}

	return list;
}

void EGX_API egx::EngineCore::EstablishDevice(const egx::Device& Device, bool UsingRenderDOC)
{
	using namespace std;
	LOG(INFOBOLD, "Establishing {0}; {1} Mb VRAM; {2} Mb Shared System RAM; Device", Device.VendorName, (double)Device.VideoRam / (1024.0 * 1024.0), (double)Device.SharedSystemRam / (1024.0 * 1024.0));
	VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };

	/*
		Storage16 and Storage8 allow uint8_t, uint16_t, float16_t (and their derivatives) to be read but no arithmetics
		shaderInt8, shaderInt16, shaderFloat16 allow arithmetics but are not supported.
	*/

	VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };
	DynamicRenderingFeature.pNext = nullptr;
	DynamicRenderingFeature.dynamicRendering = VK_TRUE;

	VkPhysicalDevice16BitStorageFeatures Storage16Bit{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES };
	Storage16Bit.pNext = &DynamicRenderingFeature;
	Storage16Bit.storageBuffer16BitAccess = VK_TRUE;
	Storage16Bit.uniformAndStorageBuffer16BitAccess = VK_TRUE;

	VkPhysicalDeviceVulkan12Features vulkan12features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	vulkan12features.pNext = &Storage16Bit;
	vulkan12features.drawIndirectCount = VK_TRUE;
	vulkan12features.shaderInt8 = VK_FALSE;
	vulkan12features.storageBuffer8BitAccess = VK_TRUE;
	vulkan12features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
	vulkan12features.scalarBlockLayout = VK_TRUE;
	vulkan12features.descriptorIndexing = VK_TRUE;
	vulkan12features.runtimeDescriptorArray = VK_TRUE;
	vulkan12features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	vulkan12features.shaderInt8 = VK_TRUE;
	vulkan12features.uniformBufferStandardLayout = VK_TRUE;
	vulkan12features.shaderInt8 = VK_TRUE;
	if (UsingRenderDOC) {
		LOG(WARNING, "Disabled bufferDeviceAddress feature so we can use RenderDOC.");
	}
	else {
		LOG(WARNING, "bufferDeviceAddress is turned on, CANNOT debug with RenderDOC, in renderdoc use 'debug' or 'renderdoc' in command line arguments.");
		vulkan12features.bufferDeviceAddress = VK_TRUE;
	}

	VkPhysicalDeviceShaderDrawParametersFeatures DrawParameters{};
	DrawParameters.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	DrawParameters.pNext = &vulkan12features;
	DrawParameters.shaderDrawParameters = VK_TRUE;
	VkPhysicalDeviceFeatures2 features{};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = &DrawParameters;
	features.features.fillModeNonSolid = VK_TRUE;
	features.features.samplerAnisotropy = VK_TRUE;
	features.features.sampleRateShading = VK_TRUE;
	features.features.multiDrawIndirect = VK_TRUE;
	features.features.shaderInt64 = VK_FALSE;
	features.features.wideLines = VK_TRUE;

	// unnecessary features.
	features.features.pipelineStatisticsQuery = VK_TRUE;
	vulkan12features.hostQueryReset = VK_TRUE;

	if (!egx::GraphicsCardFeatureValidation_Check(Device.Id, features)) {
		LOG(ERR, "{0} has failed feature validation check.", Device.VendorName);
	}
	else
		LOG(INFO, "{0} has passed feature validation check.", Device.VendorName);

	createInfo.pNext = &features;

	uint32_t QueueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device.Id, &QueueCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device.Id, &QueueCount, QueueFamilies.data());

	int index = 0;
	bool set = false;
	for (const auto& queue : QueueFamilies)
	{
		if (queue.queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT))
		{
			set = true;
			break;
		}
		index++;
	}

	if (!set)
	{
		LOG(ERR, "Could not create logical device since the Queue Flags could not be found!");
	}

	float priorities = { 1.0f };
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &priorities;
	queueCreateInfo.queueFamilyIndex = index;

	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	vkCreateDevice(Device.Id, &createInfo, NULL, &this->CoreInterface->Device);
	this->CoreInterface->QueueFamilyIndex = index;

	vkGetDeviceQueue(this->CoreInterface->Device, index, 0, &this->CoreInterface->Queue);

	this->CoreInterface->MemoryContext = VkAlloc::CreateContext(CoreInterface->Instance, this->CoreInterface->Device, Device.Id, /* 64 mb*/ 64 * (1024 * 1024));
	this->CoreInterface->PhysicalDevice = Device;
}

ImGuiContext* egx::EngineCore::GetContext() const
{
	return ImGui::GetCurrentContext();
}

static VkBool32 VKAPI_CALL ApiDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOG(WARNING, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOG(ERR, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		LOG(INFO, pCallbackData->pMessage);
	}
	else
	{
		LOG(INFOBOLD, pCallbackData->pMessage);
	}
	return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
