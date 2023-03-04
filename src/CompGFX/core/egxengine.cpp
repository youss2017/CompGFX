#include "egxengine.hpp"
#include "FeatureValidation.hpp"
#include <iostream>
#include <Utility/CppUtility.hpp>
#include <cassert>
#include <ranges>

static VkBool32 VKAPI_ATTR ApiDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
static VkBool32 DebugPrintfEXT_Callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData);

EGX_API egx::EngineCore::EngineCore(EngineCoreDebugFeatures debugFeatures, bool UsingBufferReference)
	: _Swapchain(nullptr), _CoreInterface(new VulkanCoreInterface()), UsingBufferReference(UsingBufferReference), _DebugCallbackHandle(nullptr)
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

	uint32_t count = 0;
	vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &count, nullptr);
	std::vector<VkExtensionProperties> extProps(count);
	vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &count, extProps.data());
	if (std::ranges::count_if(extProps, [](VkExtensionProperties& l) {return strcmp(l.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME); })) {
		layer_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
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
	VkValidationFeatureEnableEXT enables[] =
	{
		(debugFeatures == EngineCoreDebugFeatures::GPUAssisted) ? VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT : VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
	};
	VkValidationFeaturesEXT validationFeatures{ VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = 1;
	validationFeatures.pEnabledValidationFeatures = enables;
	debugCreateInfo.pNext = &validationFeatures;
	debugCreateInfo = {};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = ApiDebugCallback;
	createInfo.pNext = &debugCreateInfo;
#endif

	vkCreateInstance(&createInfo, NULL, &_CoreInterface->Instance);
#if defined(VK_NO_PROTOTYPES)
	volkLoadInstance(instance);
#endif
#ifdef _DEBUG
	if (CreateDebugUtilsMessengerEXT(_CoreInterface->Instance, &debugCreateInfo, nullptr, &_CoreInterface->DebugMessenger) != VK_SUCCESS)
	{
		LOG(WARNING, "Failed to set up debug messenger!");
	}
	// From https://anki3d.org/debugprintf-vulkan/
	// Populate the VkDebugReportCallbackCreateInfoEXT
	VkDebugReportCallbackCreateInfoEXT ci = {};
	ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	ci.pfnCallback = DebugPrintfEXT_Callback;
	ci.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	ci.pUserData = nullptr;

	// Create the callback handle
	PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_CoreInterface->Instance, "vkCreateDebugReportCallbackEXT");
	if (createDebugReportCallback)
		createDebugReportCallback(_CoreInterface->Instance, &ci, nullptr, &_DebugCallbackHandle);
#endif

#if defined(VK_NO_PROTOTYPES)
	volkLoadDevice(s_Context->defaultDevice);
#endif

}

EGX_API egx::EngineCore::~EngineCore()
{
	WaitIdle();
	if (_DebugCallbackHandle)
	{
		PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_CoreInterface->Instance, "vkDestroyDebugReportCallbackEXT");
		DestroyDebugReportCallbackEXT(_CoreInterface->Instance, _DebugCallbackHandle, nullptr);
	}
	if (_Swapchain)
		delete _Swapchain;
}

void EGX_API egx::EngineCore::_AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui)
{
	assert(_Swapchain == nullptr);
	// [NOTE]: ImGui multi-viewport uses VK_FORMAT_B8G8R8A8_UNORM, if we use a different format
	// [NOTE]: there will be a mismatch of format between pipeline state objects and render pass
	_CoreInterface->MaxFramesInFlight = MaxFramesInFlight;
	_Swapchain = new VulkanSwapchain(
		_CoreInterface,
		Window->GetWindow(),
		VSync,
		SetupImGui);
}

std::vector<egx::Device> EGX_API egx::EngineCore::EnumerateDevices()
{
	std::vector<egx::Device> list;
	uint32_t Count = 0;
	vkEnumeratePhysicalDevices(_CoreInterface->Instance, &Count, nullptr);
	std::vector<VkPhysicalDevice> Ids(Count);
	vkEnumeratePhysicalDevices(_CoreInterface->Instance, &Count, Ids.data());

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

		uint32_t QueueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(d.Id, &QueueCount, nullptr);
		std::vector<VkQueueFamilyProperties> QueueFamilies(QueueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(d.Id, &QueueCount, QueueFamilies.data());

		auto containsDedicatedQueue = [&](VkQueueFlags flag) -> bool {
			for (auto& queue : QueueFamilies) {
				if (queue.queueFlags & flag && !(queue.queueFlags & VK_QUEUE_GRAPHICS_BIT)) return true;
			}
			return false;
		};

		d.CardHasDedicatedCompute = containsDedicatedQueue(VK_QUEUE_COMPUTE_BIT);
		d.CardHasDedicatedTransfer = containsDedicatedQueue(VK_QUEUE_TRANSFER_BIT);

		list.push_back(d);
	}

	return list;
}

egx::ref<egx::VulkanCoreInterface> egx::EngineCore::EstablishDevice(const egx::Device& Device, const VkPhysicalDeviceFeatures2& features_)
{
	using namespace std;
	VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };

	VkPhysicalDeviceFeatures2 features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

	if (features_.sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2) {
		features = features_;
	}

	if (!egx::ValidateFeatures(Device.Id, features)) {
		LOG(ERR, "{0} has failed feature validation check.", Device.VendorName);
	}
	else
		LOG(INFO, "{0} has passed feature validation check.", Device.VendorName);

	createInfo.pNext = &features;

	uint32_t QueueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device.Id, &QueueCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device.Id, &QueueCount, QueueFamilies.data());

	int graphicsIndex = -1, transferIndex = -1, computeIndex = -1;
	std::vector<uint32_t> queueIndices;

	// extract in following order
	// 1) graphics
	// 2) compute
	// 3) transfer

	for (int j = 0; j < 3; j++) {
		int index = 0;
		for (const auto& queue : QueueFamilies)
		{
			if (graphicsIndex == -1) {
				if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicsIndex = index;
					queueIndices.push_back(index);
					_CoreInterface->GraphicsQueueFamilyIndex = index;
				}
			}
			else if (computeIndex == -1) {
				if ((queue.queueFlags & VK_QUEUE_COMPUTE_BIT) && index != graphicsIndex && index != transferIndex) {
					computeIndex = index;
					queueIndices.push_back(index);
					_CoreInterface->ComputeQueueFamilyIndex = index;
				}
			}
			else if (transferIndex == -1) {
				if ((queue.queueFlags & VK_QUEUE_TRANSFER_BIT) && index != graphicsIndex && index != computeIndex) {
					transferIndex = index;
					queueIndices.push_back(index);
					_CoreInterface->TransferQueueFamilyIndex = index;
				}
			}
			else {
				break;
			}
			index++;
		}
	}

	float priority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfo;

	for (size_t i = 0; i < queueIndices.size(); i++) {
		VkDeviceQueueCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		createInfo.queueCount = 1;
		createInfo.pQueuePriorities = &priority;
		createInfo.queueFamilyIndex = queueIndices[i];
		queueCreateInfo.push_back(createInfo);
	}

	std::vector<const char*> enabledExtensions;
	enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	enabledExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

#ifdef _DEBUG
	enabledExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	enabledExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
#endif

	createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfo.size();
	createInfo.pQueueCreateInfos = queueCreateInfo.data();
	createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();

	vkCreateDevice(Device.Id, &createInfo, NULL, &this->_CoreInterface->Device);

	vkGetDeviceQueue(_CoreInterface->Device, _CoreInterface->GraphicsQueueFamilyIndex, 0, &_CoreInterface->Queue);
	if (_CoreInterface->ComputeQueueFamilyIndex != -1)
		vkGetDeviceQueue(_CoreInterface->Device, _CoreInterface->ComputeQueueFamilyIndex, 0, &_CoreInterface->DedicatedCompute);
	if (_CoreInterface->TransferQueueFamilyIndex != -1)
		vkGetDeviceQueue(_CoreInterface->Device, _CoreInterface->TransferQueueFamilyIndex, 0, &_CoreInterface->DedicatedTransfer);

	if (!_CoreInterface->DedicatedCompute) _CoreInterface->DedicatedCompute = _CoreInterface->Queue;
	if (!_CoreInterface->DedicatedTransfer) _CoreInterface->DedicatedTransfer = _CoreInterface->Queue;

	_CoreInterface->MemoryContext = VkAlloc::CreateContext(_CoreInterface->Instance,
		_CoreInterface->Device, Device.Id, /* 64 mb*/ 64 * (1024 * 1024), UsingBufferReference);
	_CoreInterface->PhysicalDevice = Device;
	_CoreInterface->MaxFramesInFlight = 1;
	_CoreInterface->Features = features;
	return _CoreInterface;
}

ImGuiContext* egx::EngineCore::GetContext() const
{
	return ImGui::GetCurrentContext();
}

static VkBool32 VKAPI_ATTR ApiDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
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

static VkBool32 DebugPrintfEXT_Callback(VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData)
{
	LOG(INFO, "{0:%s}({1:%d}):{2:%s}", pLayerPrefix, messageCode, pMessage);
	return false;
}

cpp::Logger* egx::EngineCore::GetEngineLogger()
{
	return &cpp::Logger::GetGlobalLogger();
}
