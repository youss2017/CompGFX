#include "egx.hpp"
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

using namespace egx;
using namespace std;

VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{

	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOG(WARNING, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOG(ERR, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		LOG(INFO, pCallbackData->pMessage);
	}
	else
	{
		LOG(INFOBOLD, pCallbackData->pMessage);
	}
	return VK_FALSE;
}

shared_ptr<VulkanICDState> VulkanICDState::Create(const std::string& pApplicationName,
	bool enableValidation,
	bool useGpuAssistedValidation,
	uint32_t vkApiVersion,
	cpp::Logger* pOptionalLogger,
	PFN_vkDebugUtilsMessengerCallbackEXT pCustomCallback)
{
	return std::shared_ptr<VulkanICDState>(new VulkanICDState(pApplicationName, enableValidation, useGpuAssistedValidation, vkApiVersion, pOptionalLogger, pCustomCallback));
}

egx::VulkanICDState::VulkanICDState(const std::string& pApplicationName, bool enableValidation, bool useGpuAssistedValidation, uint32_t vkApiVersion, cpp::Logger* pOptionalLogger, PFN_vkDebugUtilsMessengerCallbackEXT pCustomCallback)
{
	if (useGpuAssistedValidation && !enableValidation)
		throw runtime_error("You cannot have gpu assisted validation without validation.");

	m_ApplicationName = pApplicationName;
	m_ValidationEnabled = enableValidation;
	m_GPUAssistedValidation = useGpuAssistedValidation;
	m_ApiVersion = vkApiVersion;
	pDebugCallback = pCustomCallback;

	if (!pOptionalLogger)
	{
		pOptionalLogger = &cpp::Logger::GetGlobalLogger();
	}
	this->pOptionalLogger = pOptionalLogger;

	if (!pCustomCallback)
	{
		pDebugCallback = DefaultDebugCallback;
	}

	// Define the application info
	vk::ApplicationInfo appInfo(
		pApplicationName.c_str(), // Application name
		VK_MAKE_VERSION(1, 0, 0), // Application version
		"EGX",					  // Engine name
		VK_MAKE_VERSION(1, 0, 0), // Engine version
		vkApiVersion			  // Vulkan API version
	);

	vector<const char*> enabledLayers;
	vector<const char*> enabledExtensions;

	glfwInit();
	uint32_t count;
	auto windowExtensions = glfwGetRequiredInstanceExtensions(&count);
	for (uint32_t i = 0; i < count; i++) {
		enabledExtensions.push_back(windowExtensions[i]);
	}

	if (enableValidation)
	{
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
	}

	// Define the instance create info
	vk::InstanceCreateInfo createInfo(
		{},									// Flags
		&appInfo,							// Pointer to application info
		uint32_t(enabledLayers.size()),		// Number of enabled layers
		enabledLayers.data(),				// Enabled layers
		uint32_t(enabledExtensions.size()), // Number of enabled extensions
		enabledExtensions.data()			// Enabled extensions
	);

	vk::ValidationFeaturesEXT validationFeatures{};
	array<vk::ValidationFeatureEnableEXT, 3> featureList = {
		vk::ValidationFeatureEnableEXT::eGpuAssisted,
		vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
		vk::ValidationFeatureEnableEXT::eBestPractices };

	if (m_GPUAssistedValidation)
	{
		validationFeatures.setEnabledValidationFeatureCount((uint32_t)featureList.size());
		validationFeatures.pEnabledValidationFeatures = featureList.data();
		createInfo.setPNext(&validationFeatures);
	}

	// Create the Vulkan instance
	if (vk::createInstance(&createInfo, nullptr, &m_Instance) != vk::Result::eSuccess)
	{
		throw runtime_error("Could not create vulkan ICD instance. Does your device support vulkan.");
	}

	if (enableValidation) {
		// Set up the debug messenger
		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
		debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		debugCreateInfo.pfnUserCallback = pDebugCallback;
		debugCreateInfo.pUserData = this;

		// Register the debug messenger
		vk::DispatchLoaderDynamic dldy(m_Instance, vkGetInstanceProcAddr);
		m_DebugMessengerEXT = m_Instance.createDebugUtilsMessengerEXT(debugCreateInfo, nullptr, dldy);
	}
}

egx::VulkanICDState::~VulkanICDState()
{
	vk::DispatchLoaderDynamic dldy(m_Instance, vkGetInstanceProcAddr);
	m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessengerEXT, nullptr, dldy);
	m_Instance.destroy();
}

std::vector<PhysicalDeviceAndQueueFamilyInfo> egx::VulkanICDState::QueryGPGPUDevices()
{
	auto instance = m_Instance;
	std::vector<PhysicalDeviceAndQueueFamilyInfo> deviceAndQueueFamilyInfo;

	std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

	for (const vk::PhysicalDevice& physicalDevice : physicalDevices)
	{
		std::vector<std::pair<uint32_t, std::vector<vk::QueueFlags>>> queueFamilyInfo;

		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
		{
			const vk::QueueFamilyProperties& properties = queueFamilyProperties[i];

			if (properties.queueCount > 0)
			{
				std::vector<vk::QueueFlags> supportedQueues;

				if (properties.queueFlags & vk::QueueFlagBits::eGraphics)
				{
					supportedQueues.push_back(vk::QueueFlagBits::eGraphics);
				}
				if (properties.queueFlags & vk::QueueFlagBits::eCompute)
				{
					supportedQueues.push_back(vk::QueueFlagBits::eCompute);
				}
				if (properties.queueFlags & vk::QueueFlagBits::eTransfer)
				{
					supportedQueues.push_back(vk::QueueFlagBits::eTransfer);
				}
				if (properties.queueFlags & vk::QueueFlagBits::eSparseBinding)
				{
					supportedQueues.push_back(vk::QueueFlagBits::eSparseBinding);
				}

				if (!supportedQueues.empty())
				{
					queueFamilyInfo.push_back({ i, supportedQueues });
				}
			}
		}

		auto deviceProperties = physicalDevice.getProperties();
		PhysicalDeviceAndQueueFamilyInfo physicalDeviceInfo;
		strncpy(physicalDeviceInfo.Name, deviceProperties.deviceName.data(), deviceProperties.deviceName.size());
		physicalDeviceInfo.QueueFamilyInfo = queueFamilyInfo;
		physicalDeviceInfo.Type = deviceProperties.deviceType;
		physicalDeviceInfo.PhysicalDevice = physicalDevice;

		deviceAndQueueFamilyInfo.push_back(physicalDeviceInfo);
	}

	vector<PhysicalDeviceAndQueueFamilyInfo> query(deviceAndQueueFamilyInfo.size());
	for (size_t i = 0; i < deviceAndQueueFamilyInfo.size(); i++)
	{
		memcpy(query[i].Name, deviceAndQueueFamilyInfo[i].Name, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
		query[i].PhysicalDevice = deviceAndQueueFamilyInfo[i].PhysicalDevice;
		query[i].Type = deviceAndQueueFamilyInfo[i].Type;
		query[i].QueueFamilyInfo = deviceAndQueueFamilyInfo[i].QueueFamilyInfo;
		query[i].SupportSwapchain = query[i].SupportExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	return query;
}

DeviceCtx egx::VulkanICDState::CreateDevice(const PhysicalDeviceAndQueueFamilyInfo& deviceQuery, uint32_t max_frames_in_flight)
{
	int32_t graphics = -1, compute = -1, transfer = -1;
	for (auto& [index, queues] : deviceQuery.QueueFamilyInfo)
	{
		if (graphics == -1)
		{
			if (ranges::any_of(queues, [](auto& x)
				{ return x == vk::QueueFlagBits::eGraphics; }))
			{
				graphics = index;
				continue;
			}
		}
		if (compute == -1)
		{
			if (ranges::any_of(queues, [](auto& x)
				{ return x == vk::QueueFlagBits::eCompute; }))
			{
				compute = index;
				continue;
			}
		}
		if (transfer == -1)
		{
			if (std::ranges::any_of(queues, [](auto& x)
				{ return x == vk::QueueFlagBits::eTransfer; }))
			{
				transfer = index;
				continue;
			}
		}
	}

	if (graphics == -1)
	{
		return nullptr;
	}

	float queuePriority = 1.0f;
	int queueCount = 1;

	vk::DeviceQueueCreateInfo queueCreateInfos[3];
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &queuePriority;
	queueCreateInfos[0].queueFamilyIndex = graphics;

	if (compute != -1)
	{
		queueCreateInfos[queueCount].queueCount = 1;
		queueCreateInfos[queueCount].pQueuePriorities = &queuePriority;
		queueCreateInfos[queueCount].queueFamilyIndex = compute;
		queueCount++;
	}

	if (transfer != -1)
	{
		queueCreateInfos[queueCount].queueCount = 1;
		queueCreateInfos[queueCount].pQueuePriorities = &queuePriority;
		queueCreateInfos[queueCount].queueFamilyIndex = transfer;
		queueCount++;
	}

	vector<const char*> enabledExtensions;

	if (deviceQuery.SupportSwapchain)
	{
		enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	if (m_ValidationEnabled)
	{
		enabledExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
	}

	vk::DeviceCreateInfo createInfo;
	createInfo.pNext = &deviceQuery.EnabledFeatures;
	createInfo.pQueueCreateInfos = queueCreateInfos;
	createInfo.queueCreateInfoCount = queueCount;
	createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();

	vk::Device device;
	auto result = deviceQuery.PhysicalDevice.createDevice(&createInfo, nullptr, &device);
	if (result != vk::Result::eSuccess)
	{
		LOG(ERR, vk::to_string(result));
		return nullptr;
	}

	DeviceCtx ctx = make_shared<DeviceContext>();
	ctx->PhysicalDeviceQuery = deviceQuery;
	ctx->Device = device;
	ctx->ICDState = shared_from_this();
	ctx->pLogger = pOptionalLogger;

	ctx->Queue = device.getQueue(graphics, 0);
	ctx->GraphicsQueueFamilyIndex = graphics;

	if (compute != -1)
	{
		ctx->DedicatedCompute = device.getQueue(compute, 0);
		ctx->ComputeQueueFamilyIndex = compute;
	}
	else
	{
		ctx->DedicatedCompute = ctx->Queue;
		ctx->ComputeQueueFamilyIndex = graphics;
	}

	if (transfer != -1)
	{
		ctx->DedicatedTransfer = device.getQueue(transfer, 0);
		ctx->TransferQueueFamilyIndex = transfer;
	}
	else
	{
		ctx->DedicatedTransfer = ctx->Queue;
		ctx->TransferQueueFamilyIndex = graphics;
	}

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = m_ApiVersion;
	allocatorCreateInfo.physicalDevice = ctx->PhysicalDeviceQuery.PhysicalDevice;
	allocatorCreateInfo.device = device;
	allocatorCreateInfo.instance = m_Instance;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

	vmaCreateAllocator(&allocatorCreateInfo, &ctx->Allocator);
	ctx->FramesInFlight = max_frames_in_flight;
	return ctx;
}

DeviceContext::~DeviceContext()
{
	Device.waitIdle();
	vmaDestroyAllocator(Allocator);
	Device.destroy();
}
