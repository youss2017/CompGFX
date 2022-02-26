#include "VkGraphicsCard.hpp"
#include "../../utils/Logger.hpp"
#include "../../utils/common.hpp"
#include "../../utils/StringUtils.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include "../memory/vulkan_memory.h"
#ifndef _WIN32
#include <alloca.h>
#endif
#include "VulkanLoader.h"

constexpr bool ShowVulkanExtensions = false;
constexpr bool ShowVulkanLayers = false;
constexpr bool ShowVulkanErrorBox = true;

namespace vk
{

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
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
	{
		vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (vkCreateDebugUtilsMessengerEXT != nullptr)
		{
			return vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pDebugMessenger);
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

	VkContext Gfx_CreateContext(PlatformWindow *Window, bool EnableDebug, bool ForceIntegeratedGPU,
								const std::vector<const char *> &Layers, const std::vector<const char *> &LayerExtensions,
								std::vector<const char *> LogicalDeviceExtensions, VkPhysicalDeviceFeatures2 GraphicsCardFeatures, uint32_t VulkanAPIVersion)
	{
#if defined(VK_NO_PROTOTYPES)
		Loader_LoadVulkan();
#endif
		if (EnableDebug)
		{
			logwarning("Enabling Validation Layers requires the VulkanSDK to be installed or validation .dlls/.so");
			logwarning("If VulkanSDK is not installed, then validation layers will be prevent the application from running and cause a crash.");
		}
		VkContext context = (VkContext)malloc(sizeof(_VkContext));
		assert(context);
		context->m_ApiType = 0;
		if (!context)
		{
			logerror("Could not allocate VkContext memory, this should not happpen!");
			return NULL;
		}

		uint32_t InstanceVersion;
		vkEnumerateInstanceVersion(&InstanceVersion);
		using namespace std;
		string instance_log = "Vulkan Version "s + to_string(VK_API_VERSION_MAJOR(InstanceVersion)) + "."s + to_string(VK_API_VERSION_MINOR(InstanceVersion));
		loginfos(instance_log);

		VkApplicationInfo appinfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
		appinfo.pApplicationName = "Exoab 3";
		appinfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appinfo.pEngineName = "ExoabEngine";
		appinfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appinfo.apiVersion = VulkanAPIVersion;
		VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		createInfo.pApplicationInfo = &appinfo;
		createInfo.enabledLayerCount = Layers.size();
		createInfo.ppEnabledLayerNames = Layers.data();
		createInfo.enabledExtensionCount = LayerExtensions.size();
		createInfo.ppEnabledExtensionNames = LayerExtensions.data();
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (EnableDebug)
		{
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
		}

		VkInstance instance = NULL;
		vkcheck(vkCreateInstance(&createInfo, NULL, &instance));
#if defined(VK_NO_PROTOTYPES)
		Loader_LoadInstance(instance);
#endif
		if (EnableDebug)
		{
			VkDebugUtilsMessengerEXT debugMessenger;
			if (CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
			{
				log_alert("Failed to set up debug messenger!", true);
			}
			context->debugMessenger = debugMessenger;
		}
		context->instance = instance;
		context->window = Window;

		GraphicsCard card = Gfx_GetDefaultCard(context, ForceIntegeratedGPU);
		memcpy(&context->card, &card, sizeof(GraphicsCard)); // because of c++ struct constructor stuff
		strcpy(context->card.name, card.name);
		loginfo(card.name);
		/* Create Default Vulkan Logical Device */
		context->defaultDevice = Gfx_CreateDevice(card,
												  LogicalDeviceExtensions,
												  (int *)&context->defaultQueueFamilyIndex,
												  VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT,
												  1,
												  GraphicsCardFeatures);
#if defined(VK_NO_PROTOTYPES)
		Loader_LoadDevice(context->defaultDevice);
#endif
		vkGetDeviceQueue(context->defaultDevice, context->defaultQueueFamilyIndex, 0, &context->defaultQueue);

		context->debugEnabled = EnableDebug;
		VkSampleCountFlags counts = card.deviceLimits.framebufferColorSampleCounts & card.deviceLimits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT)
		{
			loginfo("Max Supported MSAA Samples 64");
			context->m_MaxMSAASamples = 64;
		}
		else if (counts & VK_SAMPLE_COUNT_32_BIT)
		{
			loginfo("Max Supported MSAA Samples 32");
			context->m_MaxMSAASamples = 32;
		}
		else if (counts & VK_SAMPLE_COUNT_16_BIT)
		{
			loginfo("Max Supported MSAA Samples 16");
			context->m_MaxMSAASamples = 16;
		}
		else if (counts & VK_SAMPLE_COUNT_8_BIT)
		{
			loginfo("Max Supported MSAA Samples 8");
			context->m_MaxMSAASamples = 8;
		}
		else if (counts & VK_SAMPLE_COUNT_4_BIT)
		{
			loginfo("Max Supported MSAA Samples 4");
			context->m_MaxMSAASamples = 4;
		}
		else if (counts & VK_SAMPLE_COUNT_2_BIT)
		{
			loginfo("Max Supported MSAA Samples 2");
			context->m_MaxMSAASamples = 2;
		}
		else
		{
			loginfo("MSAA is not supported on this graphics card.");
			context->m_MaxMSAASamples = 1;
		}

		/* Allocation Callbacks */
		// VkAllocationCallbacks* allocation_callback = new VkAllocationCallbacks;
		// allocation_callback->pUserData = NULL; // void*
		// allocation_callback->pfnAllocation = NULL; // PFN_vkAllocationFunction
		// allocation_callback->pfnReallocation = NULL; // PFN_vkReallocationFunction
		// allocation_callback->pfnFree = NULL; // PFN_vkFreeFunction
		// allocation_callback->pfnInternalAllocation = NULL; // PFN_vkInternalAllocationNotification
		// allocation_callback->pfnInternalFree = NULL; // PFN_vkInternalFreeNotification
		context->m_allocation_callback = NULL;

		context->m_future_memory_context = VkAlloc::CreateContext(context->instance, context->defaultDevice, context->card.handle, /* 64 mb*/ 64 * (1024 * 1024));

		return context;
	}

	void Gfx_DestroyContext(VkContext context)
	{
		vkDeviceWaitIdle(context->defaultDevice);
		VkAlloc::DestroyContext(context->m_future_memory_context);
		vkDestroyDevice(context->defaultDevice, context->m_allocation_callback);
		if (context->debugEnabled)
			DestroyDebugUtilsMessengerEXT(context->instance, context->debugMessenger, nullptr);
		vkDestroyInstance(context->instance, nullptr);
		if (context->m_allocation_callback)
			delete context->m_allocation_callback;
		free(context);
	}

	std::vector<GraphicsCard> Gfx_GetAllGraphicsCards(VkContext context)
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

	GraphicsCard Gfx_GetDefaultCard(VkContext context, bool forceIntegratedGPU)
	{
		std::vector<GraphicsCard> cards = Gfx_GetAllGraphicsCards(context);
		GraphicsCard card = cards[0];
		if (!forceIntegratedGPU)
		{
			for (const auto &t : cards)
			{
				if (t.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					if (card.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					{
						if (card.memorySize < t.memorySize)
							card = t;
					}
					else
					{
						card = t;
					}
				}
			}
		}
		else
		{
			for (const auto &t : cards)
			{
				if (t.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				{
					card = t;
					break;
				}
			}
		}
		if (card.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			logalert("Selected an Integrated GPU");
		}
		return card;
	}

	VkDevice Gfx_CreateDevice(GraphicsCard &card, std::vector<const char *> enabledExtensions, int *FamilyQueueIndex, VkQueueFlags queueFlags, int queueCount, VkPhysicalDeviceFeatures2 enabledFeatures)
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

	VkAttachmentDescription Gfx_InitImageAttachment(
		VkFormat imageFormat, VkImageLayout InitialLayout, VkImageLayout FinalLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
		VkAttachmentLoadOp loadStencilOp, VkAttachmentStoreOp storeStencilOp, VkSampleCountFlagBits samples)
	{
		VkAttachmentDescription attachment{};
		attachment.format = imageFormat;
		attachment.samples = samples;
		attachment.loadOp = loadOp;
		attachment.storeOp = storeOp;
		attachment.stencilLoadOp = loadStencilOp;
		attachment.stencilStoreOp = storeStencilOp;
		attachment.initialLayout = InitialLayout;
		attachment.finalLayout = FinalLayout;
		return attachment;
	}

	VkRenderPass CreateRenderPass(VkContext context, std::vector<VkAttachmentDescription> &attachments, std::vector<VkSubpassDescription> &subpasses)
	{
		VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		createInfo.attachmentCount = attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = subpasses.size();
		createInfo.pSubpasses = subpasses.data();

		VkRenderPass renderPass;
		vkcheck(vkCreateRenderPass(context->defaultDevice, &createInfo, context->m_allocation_callback, &renderPass));
		return renderPass;
	}

	VkFence Gfx_CreateFence(VkContext context, bool signaled)
	{
		VkFenceCreateInfo createInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
		createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
		VkFence fence;
		vkcheck(vkCreateFence(context->defaultDevice, &createInfo, context->m_allocation_callback, &fence));
		return fence;
	}

	VkSemaphore Gfx_CreateSemaphore(VkContext context, bool TimelineSemaphore)
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
		vkcheck(vkCreateSemaphore(context->defaultDevice, &createInfo, context->m_allocation_callback, &semaphore));
		return semaphore;
	}

	VkCommandPool Gfx_CreateCommandPool(VkContext context, bool memoryShortLived, bool enableIndividualReset, bool makeProtected)
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
		vkCreateCommandPool(context->defaultDevice, &createInfo, context->m_allocation_callback, &pool);
		return pool;
	}

	VkCommandBuffer Gfx_AllocCommandBuffer(VkContext context, VkCommandPool pool, bool primaryLevel)
	{
		VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocInfo.commandPool = pool;
		allocInfo.level = primaryLevel ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;
		VkCommandBuffer buffer;
		vkcheck(vkAllocateCommandBuffers(context->defaultDevice, &allocInfo, &buffer));
		return buffer;
	}

	void Gfx_StartCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage)
	{
		VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		beginInfo.flags = usage;
		vkcheck(vkBeginCommandBuffer(buffer, &beginInfo));
	}

#if 0
	void Gfx_StartRenderPass(VkCommandBuffer buffer, Framebuffer *FBO, uint32_t FrameIndex, int ClearValuesCount, VkClearValue *pClearValues)
	{
		VkRenderPassBeginInfo BeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		auto FBOSpec = FBO->GetSpecification();
		BeginInfo.renderPass = FBO->GetRenderPass().GetNativeHandle();
		BeginInfo.renderArea.extent = {(u32)FBOSpec.Width, (u32)FBOSpec.Height};
		BeginInfo.framebuffer = (VkFramebuffer)FBO->GetAPINativeHandle(FrameIndex);
		if (FBOSpec.Samples == TextureSamples::MSAA_1)
		{
			BeginInfo.clearValueCount = ClearValuesCount;
			BeginInfo.pClearValues = pClearValues;
		}
		else
		{
			// Generate Clear Values
			const auto &refs = FBO->m_AttachmentReferences;
			int ClearValueCount = refs.size();
			VkClearValue *pActualClearValues = (VkClearValue *)alloca(sizeof(VkClearValue) * ClearValueCount);
			assert(pActualClearValues);
			int q = 0;
			for (int i = 0; i < ClearValueCount; i += 2)
			{
				if (refs[i].layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
				{
					pActualClearValues[i] = pClearValues[q];
					pActualClearValues[i + 1] = pClearValues[q];
				}
				else if (refs[i].layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					q++;
					continue;
				}
				else
				{
					assert(0 && "Implement me!");
				}
				q++;
			}
			q = 0;
			for (int i = 0; i < ClearValueCount; i += 2)
			{
				if (refs[i].layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					pActualClearValues[i] = pClearValues[q];
				}
				q++;
			}
			BeginInfo.clearValueCount = ClearValueCount;
			BeginInfo.pClearValues = pActualClearValues;
		}

		vkCmdBeginRenderPass(buffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
#endif

	void Gfx_StartRenderPass(VkCommandBuffer buffer, VkRenderPass RenderPass, VkFramebuffer fbo, int FboWidth, int FboHeight, int ClearValuesCount, VkClearValue *pClearValues)
	{
		VkRenderPassBeginInfo BeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		BeginInfo.renderPass = RenderPass;
		BeginInfo.renderArea.extent = {(uint32_t)FboWidth, (uint32_t)FboHeight};
		BeginInfo.framebuffer = fbo;
		BeginInfo.clearValueCount = ClearValuesCount;
		BeginInfo.pClearValues = pClearValues;
		vkCmdBeginRenderPass(buffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	/*************************************************** PIPELINE LAYOUT ***************************************************/

	VkPushConstantRange Gfx_InitPushConstantRange(VkShaderStageFlags shaderStage, int offset, int size)
	{
		VkPushConstantRange vkrange{};
		vkrange.stageFlags = shaderStage;
		vkrange.offset = offset;
		vkrange.size = size;
		return vkrange;
	}

	VkDescriptorPool Gfx_CreateDescriptorPool(VkContext context, int maxSets, const std::vector<VkDescriptorPoolSize> &poolSizes, VkDescriptorPoolCreateFlags flags)
	{
		VkDescriptorPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		createInfo.flags = flags;
		createInfo.maxSets = maxSets;
		createInfo.poolSizeCount = poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();
		VkDescriptorPool pool;
		vkcheck(vkCreateDescriptorPool(context->defaultDevice, &createInfo, context->m_allocation_callback, &pool));
		return pool;
	}

	VkSampler Gfx_CreateSampler(VkContext context, 
		VkFilter magFilter, 
		VkFilter minFilter, 
		VkSamplerMipmapMode mipmapMode, 
		VkSamplerAddressMode u, 
		VkSamplerAddressMode v, 
		VkSamplerAddressMode w, 
		float mipLodBias,
		VkBool32 anisotropyEnable,
		float maxAnisotropy,
		float minLod,
		float maxLod)
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
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		VkSampler sampler;
		vkCreateSampler(context->defaultDevice, &createInfo, nullptr, &sampler);
		return sampler;
	}


	void Gfx_SubmitCmdBuffers(VkQueue queue, std::vector<VkCommandBuffer> cmdBuffers, std::vector<VkSemaphore> waitSemaphores, std::vector<VkPipelineStageFlags> waitDstStageMask, std::vector<VkSemaphore> signalSemaphores, VkFence fence)
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

	size_t PadUniformBuffer(VkContext context, size_t struct_size)
	{
		// Calculate required alignment based on minimum device offset alignment
		size_t minUboAlignment = context->card.deviceLimits.minUniformBufferOffsetAlignment;
		size_t alignedSize = struct_size;
		if (minUboAlignment > 0)
		{
			alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		return alignedSize;
	}

	std::string GetVkResultString(VkResult result)
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

	uint32_t AlignSizeToUniformAlignment(GraphicsContext _context, uint32_t size)
	{
		if (*(char *)_context == 0)
		{
			vk::VkContext context = ToVKContext(_context);
			uint32_t alignment = context->card.deviceLimits.minUniformBufferOffsetAlignment;
			uint32_t aligned_size = size;
			if (aligned_size % alignment != 0)
			{
				int offset = alignment - (aligned_size - ((aligned_size % alignment) * alignment));
				aligned_size += offset;
			}
			return aligned_size;
		}
		else
			return size;
	}

}