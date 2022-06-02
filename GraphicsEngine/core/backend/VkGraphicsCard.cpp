#include "VkGraphicsCard.hpp"
#include "../../utils/Logger.hpp"
#include "../../utils/common.hpp"
#include "../../utils/StringUtils.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#ifndef _WIN32
#include <alloca.h>
#endif
#include "VulkanLoader.h"
#pragma comment(lib, "vulkan-1.lib")

constexpr bool ShowVulkanExtensions = false;
constexpr bool ShowVulkanLayers = false;
constexpr bool ShowVulkanErrorBox = false;

VkBool32 GraphicsCardFeatureValidation_Check(VkPhysicalDevice device, VkPhysicalDeviceFeatures2 requiredFeatures);

namespace vk
{

	struct CONFIGURATION {
		PlatformWindow* Window;
		bool EnableDebug;
		bool ForceIntegeratedGPU;
		std::vector<const char*> Layers;
		std::vector<const char*> LayerExtensions;
		std::vector<const char*> LogicalDeviceExtensions;
		VkPhysicalDeviceFeatures2 GraphicsCardFeatures;
		uint32_t VulkanAPIVersion;
	};

	static bool s_Configured = false;
	static CONFIGURATION s_Config;
	static VkContext s_Context = nullptr;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
	{
		if (Utils::StrStartsWith(std::string(pCallbackData->pMessage), "loader_scanned_icd_add: Driver"))
			return VK_FALSE;
		if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			logwarning(pCallbackData->pMessage);
		}
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			if (ShowVulkanErrorBox)
			{
				if (!log_error(pCallbackData->pMessage, "N/A", 0))
					Utils::Break();
			}
			else
			{
				log_error(pCallbackData->pMessage, "N/A", 0, false);
			}
		}
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			loginfo(pCallbackData->pMessage);
		}
		else
		{
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		}
		return VK_FALSE;
	}

	static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
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

	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	void Gfx_DestroyInstance(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
	{
		if (debugMessenger)
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		vkDestroyInstance(instance, NULL);
	}

	void GRAPHICS_API Gfx_ConfigureContext(PlatformWindow* Window, bool EnableDebug, bool ForceIntegeratedGPU, const std::vector<const char*>& Layers, const std::vector<const char*>& LayerExtensions, std::vector<const char*> LogicalDeviceExtensions, VkPhysicalDeviceFeatures2 GraphicsCardFeatures, uint32_t VulkanAPIVersion)
	{
		if (s_Configured) {
			log_error("Context Already Configured. CANNOT Be configured again", __FILE__, __LINE__, true);
			return;
		}
		s_Configured = true;
		s_Config.Window = Window;
		s_Config.EnableDebug = EnableDebug;
		s_Config.ForceIntegeratedGPU = ForceIntegeratedGPU;
		s_Config.Layers = Layers;
		s_Config.LayerExtensions = LayerExtensions;
		s_Config.GraphicsCardFeatures = GraphicsCardFeatures;
		s_Config.LogicalDeviceExtensions = LogicalDeviceExtensions;
		s_Config.VulkanAPIVersion = VulkanAPIVersion;
	}

	std::vector<GraphicsCard> GRAPHICS_API Gfx_GetAllGraphicsCards(VkContext context);
	GraphicsCard GRAPHICS_API Gfx_GetDefaultCard(VkContext context, bool ForceIntegratedGPU, VkPhysicalDeviceFeatures2 requiredFeatures);
	VkDevice GRAPHICS_API Gfx_CreateDevice(GraphicsCard& card, std::vector<const char*> enabledExtensions, int* FamilyQueueIndex, VkQueueFlags queueFlags, int queueCount, VkPhysicalDeviceFeatures2 enabledFeatures);

	VkContext GRAPHICS_API Gfx_GetContext()
	{
		if (s_Context)
			return s_Context;
		if (!s_Configured) {
			log_error("Please Call Gfx_ConfigureContext First!", __FILE__, __LINE__, true);
			return nullptr;
		}
#if defined(VK_NO_PROTOTYPES)
		Loader_LoadVulkan();
#endif
		if (s_Config.EnableDebug)
		{
			logwarning("Enabling Validation Layers requires the VulkanSDK to be installed or validation .dlls/.so");
			logwarning("If VulkanSDK is not installed, then validation layers will be prevent the application from running and cause a crash.");
		}
		s_Context = (_VkContext*)new _VkContext;
		s_Context->m_ApiType = 0;

		uint32_t InstanceVersion;
		vkEnumerateInstanceVersion(&InstanceVersion);
		using namespace std;
		string instance_log = "Vulkan Version "s + to_string(VK_API_VERSION_MAJOR(InstanceVersion)) + "."s + to_string(VK_API_VERSION_MINOR(InstanceVersion));
		loginfos(instance_log);

		VkApplicationInfo appinfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appinfo.pApplicationName = "Application";
		appinfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appinfo.pEngineName = "Application";
		appinfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appinfo.apiVersion = s_Config.VulkanAPIVersion;
		VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		createInfo.pApplicationInfo = &appinfo;
		createInfo.enabledLayerCount = s_Config.Layers.size();
		createInfo.ppEnabledLayerNames = s_Config.Layers.data();
		createInfo.enabledExtensionCount = s_Config.LayerExtensions.size();
		createInfo.ppEnabledExtensionNames = s_Config.LayerExtensions.data();
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (s_Config.EnableDebug)
		{
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}

		VkInstance instance = NULL;
		vkcheck(vkCreateInstance(&createInfo, NULL, &instance));
#if defined(VK_NO_PROTOTYPES)
		Loader_LoadInstance(instance);
#endif
		if (s_Config.EnableDebug)
		{
			VkDebugUtilsMessengerEXT debugMessenger;
			if (CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
			{
				log_alert("Failed to set up debug messenger!", true);
			}
			s_Context->debugMessenger = debugMessenger;
		}
		s_Context->instance = instance;
		s_Context->window = s_Config.Window;

		GraphicsCard card = Gfx_GetDefaultCard(s_Context, s_Config.ForceIntegeratedGPU, s_Config.GraphicsCardFeatures);
		memcpy(&s_Context->card, &card, sizeof(GraphicsCard)); // because of c++ struct constructor stuff
		strcpy(s_Context->card.name, card.name);
		loginfo(card.name);
		/* Create Default Vulkan Logical Device */
		s_Context->defaultDevice = nullptr;
		s_Context->defaultDevice = Gfx_CreateDevice(card,
			s_Config.LogicalDeviceExtensions,
			(int*)&s_Context->defaultQueueFamilyIndex,
			VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT,
			1,
			s_Config.GraphicsCardFeatures);
#if defined(VK_NO_PROTOTYPES)
		Loader_LoadDevice(s_Context->defaultDevice);
#endif
		vkGetDeviceQueue(s_Context->defaultDevice, s_Context->defaultQueueFamilyIndex, 0, &s_Context->defaultQueue);

		s_Context->debugEnabled = s_Config.EnableDebug;
		VkSampleCountFlags counts = card.deviceLimits.framebufferColorSampleCounts & card.deviceLimits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT)
		{
			loginfo("Max Supported MSAA Samples 64");
			s_Context->m_MaxMSAASamples = 64;
		}
		else if (counts & VK_SAMPLE_COUNT_32_BIT)
		{
			loginfo("Max Supported MSAA Samples 32");
			s_Context->m_MaxMSAASamples = 32;
		}
		else if (counts & VK_SAMPLE_COUNT_16_BIT)
		{
			loginfo("Max Supported MSAA Samples 16");
			s_Context->m_MaxMSAASamples = 16;
		}
		else if (counts & VK_SAMPLE_COUNT_8_BIT)
		{
			loginfo("Max Supported MSAA Samples 8");
			s_Context->m_MaxMSAASamples = 8;
		}
		else if (counts & VK_SAMPLE_COUNT_4_BIT)
		{
			loginfo("Max Supported MSAA Samples 4");
			s_Context->m_MaxMSAASamples = 4;
		}
		else if (counts & VK_SAMPLE_COUNT_2_BIT)
		{
			loginfo("Max Supported MSAA Samples 2");
			s_Context->m_MaxMSAASamples = 2;
		}
		else
		{
			loginfo("MSAA is not supported on this graphics card.");
			s_Context->m_MaxMSAASamples = 1;
		}

		s_Context->m_future_memory_context = VkAlloc::CreateContext(s_Context->instance, s_Context->defaultDevice, s_Context->card.handle, /* 64 mb*/ 64 * (1024 * 1024));

		return s_Context;
	}

	void GRAPHICS_API Gfx_ReleaseContext()
	{
		if (!s_Context) {
			log_error("Context has not been created yet.", __FILE__, __LINE__, true);
			return;
		}
		vkDeviceWaitIdle(s_Context->defaultDevice);
		VkAlloc::DestroyContext(s_Context->m_future_memory_context);
		vkDestroyDevice(s_Context->defaultDevice, 0);
		if (s_Context->debugEnabled)
			DestroyDebugUtilsMessengerEXT(s_Context->instance, s_Context->debugMessenger, nullptr);
		vkDestroyInstance(s_Context->instance, nullptr);
		// For some reason this is broken?
		//delete context;
		s_Context = nullptr;
	}

	std::vector<GraphicsCard> GRAPHICS_API Gfx_GetAllGraphicsCards(VkContext context)
	{
		uint32_t deviceCount;
		vkcheck(vkEnumeratePhysicalDevices(context->instance, &deviceCount, NULL));
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkcheck(vkEnumeratePhysicalDevices(context->instance, &deviceCount, devices.data()));

		std::vector<GraphicsCard> cards;
		for (uint32_t i = 0; i < deviceCount; i++)
		{
			VkPhysicalDeviceProperties prop;
			vkGetPhysicalDeviceProperties(devices[i], &prop);
			VkPhysicalDeviceMemoryProperties memProp;
			vkGetPhysicalDeviceMemoryProperties(devices[i], &memProp);
			GraphicsCard card;
			strcpy(card.name, prop.deviceName);
			card.memorySize = memProp.memoryHeaps[0].size;
			card.deviceType = prop.deviceType;
			card.deviceLimits = prop.limits;
			card.handle = devices[i];
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(card.handle, &queueFamilyCount, NULL);
			card.QueueFamilies.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(card.handle, &queueFamilyCount, card.QueueFamilies.data());
			cards.push_back(card);
		}
		return cards;
	}

	GraphicsCard GRAPHICS_API Gfx_GetDefaultCard(VkContext context, bool forceIntegratedGPU, VkPhysicalDeviceFeatures2 requiredFeatures)
	{
		std::vector<GraphicsCard> cards = Gfx_GetAllGraphicsCards(context);
		GraphicsCard card = cards[0];
			for (const auto &t : cards)
			{
				if (!GraphicsCardFeatureValidation_Check(t.handle, requiredFeatures)) {
					continue;
				}
				
				card = t;
			}
	
		if (card.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			logalert("Selected an Integrated GPU");
		}
		return card;
	}

	VkDevice GRAPHICS_API Gfx_CreateDevice(GraphicsCard &card, std::vector<const char *> enabledExtensions, int *FamilyQueueIndex, VkQueueFlags queueFlags, int queueCount, VkPhysicalDeviceFeatures2 enabledFeatures)
	{
		VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
		VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
		
		createInfo.pNext = &enabledFeatures;

		int index = 0;
		bool set = false;
		for (const auto &queue : card.QueueFamilies)
		{
			if (queue.queueFlags | queueFlags)
			{
				set = true;
				break;
			}
			index++;
		}

		if (!set)
		{
			logalert("Could not create logical device since the Queue Flags could not be found!");
			return nullptr;
		}

		if (FamilyQueueIndex)
		{
			*FamilyQueueIndex = index;
		}

		float *priorities = new float[queueCount];
		for (int i = 0; i < queueCount; i++)
			priorities[i] = 1.0f;
		queueCreateInfo.queueCount = queueCount;
		queueCreateInfo.pQueuePriorities = priorities;
		queueCreateInfo.queueFamilyIndex = index;

		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.enabledExtensionCount = enabledExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
		//createInfo.pEnabledFeatures = enabledFeatures;

		VkDevice device;
		vkcheck(vkCreateDevice(card.handle, &createInfo, NULL, &device));

		/* For Debugging purposes */
		uint32_t PropertyCount = 0;
		vkEnumerateDeviceExtensionProperties(card.handle, NULL, &PropertyCount, NULL);
		std::vector<VkExtensionProperties> ExtensionProperties(PropertyCount);
		vkEnumerateDeviceExtensionProperties(card.handle, NULL, &PropertyCount, ExtensionProperties.data());

#if 0
		printf("================= DEVICE EXTENSIONS =================\n");
		for(int i = 0; i < ExtensionProperties.size(); i++)
			printf("%s --- \n", ExtensionProperties[i].extensionName, ExtensionProperties[i].specVersion);
#endif
		return device;
	}

	VkFence GRAPHICS_API Gfx_CreateFence(bool signaled)
	{
		VkFenceCreateInfo createInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
		createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
		VkFence fence;
		vkcheck(vkCreateFence(s_Context->defaultDevice, &createInfo, 0, &fence));
		return fence;
	}

	VkSemaphore GRAPHICS_API Gfx_CreateSemaphore(bool TimelineSemaphore)
	{
		VkSemaphoreTypeCreateInfo timelineCreateInfo;
		timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timelineCreateInfo.pNext = NULL;
		timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineCreateInfo.initialValue = 0;

		VkSemaphoreCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.pNext = TimelineSemaphore ? &timelineCreateInfo : nullptr;
		createInfo.flags = 0;
		VkSemaphore semaphore;
		vkcheck(vkCreateSemaphore(s_Context->defaultDevice, &createInfo, 0, &semaphore));
		return semaphore;
	}

	VkCommandPool GRAPHICS_API Gfx_CreateCommandPool(bool memoryShortLived, bool enableIndividualReset, bool makeProtected)
	{
		VkCommandPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
		if (memoryShortLived)
		{
			createInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		}
		if (enableIndividualReset)
		{
			createInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		}
		if (makeProtected)
		{
			createInfo.flags |= VK_COMMAND_POOL_CREATE_PROTECTED_BIT;
		}
		VkCommandPool pool;
		vkCreateCommandPool(s_Context->defaultDevice, &createInfo, 0, &pool);
		return pool;
	}

	VkCommandBuffer GRAPHICS_API Gfx_AllocCommandBuffer(VkCommandPool pool, bool primaryLevel)
	{
		VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocInfo.commandPool = pool;
		allocInfo.level = primaryLevel ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;
		VkCommandBuffer buffer;
		vkcheck(vkAllocateCommandBuffers(s_Context->defaultDevice, &allocInfo, &buffer));
		return buffer;
	}

	void GRAPHICS_API Gfx_StartCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage)
	{
		VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		beginInfo.flags = usage;
		vkcheck(vkBeginCommandBuffer(buffer, &beginInfo));
	}

	/*************************************************** PIPELINE LAYOUT ***************************************************/

	VkDescriptorPool GRAPHICS_API Gfx_CreateDescriptorPool(int maxSets, const std::vector<VkDescriptorPoolSize> &poolSizes, VkDescriptorPoolCreateFlags flags)
	{
		VkDescriptorPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		createInfo.flags = flags;
		createInfo.maxSets = maxSets;
		createInfo.poolSizeCount = poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();
		VkDescriptorPool pool;
		vkcheck(vkCreateDescriptorPool(s_Context->defaultDevice, &createInfo, 0, &pool));
		return pool;
	}

	VkSampler Gfx_CreateSampler(VkFilter magFilter, 
		VkFilter minFilter, 
		VkSamplerMipmapMode mipmapMode, 
		VkSamplerAddressMode u, 
		VkSamplerAddressMode v, 
		VkSamplerAddressMode w, 
		float mipLodBias,
		VkBool32 anisotropyEnable,
		float maxAnisotropy,
		float minLod,
		float maxLod,
		VkBorderColor borderColor)
	{
		VkSamplerCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.magFilter = magFilter;
		createInfo.minFilter = minFilter;
		createInfo.mipmapMode = mipmapMode;
		createInfo.addressModeU = u;
		createInfo.addressModeV = v;
		createInfo.addressModeW = w;
		createInfo.mipLodBias = mipLodBias;
		createInfo.anisotropyEnable = anisotropyEnable;
		createInfo.maxAnisotropy = maxAnisotropy;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.minLod = minLod;
		createInfo.maxLod = maxLod;
		createInfo.borderColor = borderColor;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		VkSampler sampler;
		vkCreateSampler(s_Context->defaultDevice, &createInfo, nullptr, &sampler);
		return sampler;
	}


	void GRAPHICS_API Gfx_SubmitCmdBuffers(VkQueue queue, std::vector<VkCommandBuffer> cmdBuffers, std::vector<VkSemaphore> waitSemaphores, std::vector<VkPipelineStageFlags> waitDstStageMask, std::vector<VkSemaphore> signalSemaphores, VkFence fence)
	{
		VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submitInfo.waitSemaphoreCount = waitSemaphores.size();
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitDstStageMask.data();
		submitInfo.commandBufferCount = cmdBuffers.size();
		submitInfo.pCommandBuffers = cmdBuffers.data();
		submitInfo.signalSemaphoreCount = signalSemaphores.size();
		submitInfo.pSignalSemaphores = signalSemaphores.data();
		vkcheck(vkQueueSubmit(queue, 1, &submitInfo, fence));
	}

	void Gfx_SetImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
							VkImageAspectFlags aspects,
							VkImageLayout oldLayout,
							VkImageLayout newLayout)
	{
		VkImageMemoryBarrier imageBarrier = {};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.pNext = NULL;
		imageBarrier.oldLayout = oldLayout;
		imageBarrier.newLayout = newLayout;
		imageBarrier.image = image;
		imageBarrier.subresourceRange.aspectMask = aspects;
		imageBarrier.subresourceRange.baseMipLevel = 0;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.layerCount = 1;

		switch (oldLayout)
		{
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			imageBarrier.srcAccessMask =
				VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			assert(0);
			break;
		}

		switch (newLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			imageBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			imageBarrier.dstAccessMask |=
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			imageBarrier.srcAccessMask =
				VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			assert(0);
		}
		imageBarrier.srcAccessMask = 0;
		VkPipelineStageFlagBits srcFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkPipelineStageFlagBits dstFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		vkCmdPipelineBarrier(cmdBuffer, srcFlags, dstFlags, 0, 0, NULL, 0, NULL, 1, &imageBarrier);
	}

	SingleUseCmdBuffer Gfx_CreateSingleUseCmdBuffer()
	{
		SingleUseCmdBuffer buf;
		buf.device = s_Context->defaultDevice;
		buf.queue = s_Context->defaultQueue;
		buf.fence = Gfx_CreateFence(false);
		buf.pool = Gfx_CreateCommandPool(true, true, false);
		buf.cmd = Gfx_AllocCommandBuffer(buf.pool, true);
		VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(buf.cmd, &beginInfo);
		return buf;
	}

	void Gfx_SubmitSingleUseCmdBufferAndDestroy(SingleUseCmdBuffer& buffer)
	{
		vkEndCommandBuffer(buffer.cmd);
		VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer.cmd;
		vkQueueSubmit(buffer.queue, 1, &submitInfo, buffer.fence);
		vkWaitForFences(buffer.device, 1, &buffer.fence, true, UINT64_MAX);
		vkDestroyFence(buffer.device, buffer.fence, nullptr);
		vkDestroyCommandPool(buffer.device, buffer.pool, nullptr);
	}

	VkBufferMemoryBarrier Gfx_BufferMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkBuffer buffer)
	{
		VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = buffer;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;
		return barrier;
	}

	VkQueryPool GRAPHICS_API Gfx_CreateQueryPool(VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics)
	{
		VkQueryPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
		createInfo.queryType = queryType;
		createInfo.queryCount = queryCount;
		createInfo.pipelineStatistics = pipelineStatistics;
		VkQueryPool pool;
		vkCreateQueryPool(s_Context->defaultDevice, &createInfo, nullptr, &pool);
		return pool;
	}

	std::string GRAPHICS_API GetVkResultString(VkResult result)
	{
		if (VK_SUCCESS == result)
			return std::string("VK_SUCCESS");
		if (VK_NOT_READY == result)
			return std::string("VK_NOT_READY");
		if (VK_TIMEOUT == result)
			return std::string("VK_TIMEOUT");
		if (VK_EVENT_SET == result)
			return std::string("VK_EVENT_SET");
		if (VK_EVENT_RESET == result)
			return std::string("VK_EVENT_RESET");
		if (VK_INCOMPLETE == result)
			return std::string("VK_INCOMPLETE");
		if (VK_ERROR_OUT_OF_HOST_MEMORY == result)
			return std::string("VK_ERROR_OUT_OF_HOST_MEMORY");
		if (VK_ERROR_OUT_OF_DEVICE_MEMORY == result)
			return std::string("VK_ERROR_OUT_OF_DEVICE_MEMORY");
		if (VK_ERROR_INITIALIZATION_FAILED == result)
			return std::string("VK_ERROR_INITIALIZATION_FAILED");
		if (VK_ERROR_DEVICE_LOST == result)
			return std::string("VK_ERROR_DEVICE_LOST");
		if (VK_ERROR_MEMORY_MAP_FAILED == result)
			return std::string("VK_ERROR_MEMORY_MAP_FAILED");
		if (VK_ERROR_LAYER_NOT_PRESENT == result)
			return std::string("VK_ERROR_LAYER_NOT_PRESENT");
		if (VK_ERROR_EXTENSION_NOT_PRESENT == result)
			return std::string("VK_ERROR_EXTENSION_NOT_PRESENT");
		if (VK_ERROR_FEATURE_NOT_PRESENT == result)
			return std::string("VK_ERROR_FEATURE_NOT_PRESENT");
		if (VK_ERROR_INCOMPATIBLE_DRIVER == result)
			return std::string("VK_ERROR_INCOMPATIBLE_DRIVER");
		if (VK_ERROR_TOO_MANY_OBJECTS == result)
			return std::string("VK_ERROR_TOO_MANY_OBJECTS");
		if (VK_ERROR_FORMAT_NOT_SUPPORTED == result)
			return std::string("VK_ERROR_FORMAT_NOT_SUPPORTED");
		if (VK_ERROR_FRAGMENTED_POOL == result)
			return std::string("VK_ERROR_FRAGMENTED_POOL");
		if (VK_ERROR_UNKNOWN == result)
			return std::string("VK_ERROR_UNKNOWN");
		if (VK_ERROR_OUT_OF_POOL_MEMORY == result)
			return std::string("VK_ERROR_OUT_OF_POOL_MEMORY");
		if (VK_ERROR_INVALID_EXTERNAL_HANDLE == result)
			return std::string("VK_ERROR_INVALID_EXTERNAL_HANDLE");
		if (VK_ERROR_FRAGMENTATION == result)
			return std::string("VK_ERROR_FRAGMENTATION");
		if (VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS == result)
			return std::string("VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS");
		if (VK_ERROR_SURFACE_LOST_KHR == result)
			return std::string("VK_ERROR_SURFACE_LOST_KHR");
		if (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR == result)
			return std::string("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
		if (VK_SUBOPTIMAL_KHR == result)
			return std::string("VK_SUBOPTIMAL_KHR");
		if (VK_ERROR_OUT_OF_DATE_KHR == result)
			return std::string("VK_ERROR_OUT_OF_DATE_KHR");
		if (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR == result)
			return std::string("VK_ERROR_INCOMPATIBLE_DISPLAY_KHR");
		if (VK_ERROR_VALIDATION_FAILED_EXT == result)
			return std::string("VK_ERROR_VALIDATION_FAILED_EXT");
		if (VK_ERROR_INVALID_SHADER_NV == result)
			return std::string("VK_ERROR_INVALID_SHADER_NV");
		if (VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT == result)
			return std::string("VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT");
		if (VK_ERROR_NOT_PERMITTED_EXT == result)
			return std::string("VK_ERROR_NOT_PERMITTED_EXT");
		if (VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT == result)
			return std::string("VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT");
		if (VK_THREAD_IDLE_KHR == result)
			return std::string("VK_THREAD_IDLE_KHR");
		if (VK_THREAD_DONE_KHR == result)
			return std::string("VK_THREAD_DONE_KHR");
		if (VK_OPERATION_DEFERRED_KHR == result)
			return std::string("VK_OPERATION_DEFERRED_KHR");
		if (VK_OPERATION_NOT_DEFERRED_KHR == result)
			return std::string("VK_OPERATION_NOT_DEFERRED_KHR");
		if (VK_PIPELINE_COMPILE_REQUIRED_EXT == result)
			return std::string("VK_PIPELINE_COMPILE_REQUIRED_EXT");
		if (VK_ERROR_OUT_OF_POOL_MEMORY_KHR == result)
			return std::string("VK_ERROR_OUT_OF_POOL_MEMORY_KHR");
		if (VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR == result)
			return std::string("VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR");
		if (VK_ERROR_FRAGMENTATION_EXT == result)
			return std::string("VK_ERROR_FRAGMENTATION_EXT");
		if (VK_ERROR_INVALID_DEVICE_ADDRESS_EXT == result)
			return std::string("VK_ERROR_INVALID_DEVICE_ADDRESS_EXT");
		if (VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR == result)
			return std::string("VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR");
		if (VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT == result)
			return std::string("VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT");
		if (VK_RESULT_MAX_ENUM == result)
			return std::string("VK_RESULT_MAX_ENUM");
		return std::string("UNDEFEINED or UNDETECTED VKRESULT! VkGraphicsCard.cpp");
	}

}