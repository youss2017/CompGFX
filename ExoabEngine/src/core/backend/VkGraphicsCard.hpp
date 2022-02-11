#pragma once
#include "../../window/PlatformWindow.hpp"
#include "../memory/Textures.hpp"
#include <vector>
#include <string>
#include "VulkanLoader.h"
#include <memory/vulkan_memory_allocator.hpp>


#ifndef MEM_KB
#define MEM_KB(x) (x * 1024)
#define MEM_MB(x) (MEM_KB(x) * x)
#define MEM_GB(x) (MEM_MB(x) * x)
#endif

#if defined(_DEBUG)
#define vkcheck(x)\
{\
	VkResult ___vk_i_result____ = x;\
	if(___vk_i_result____  != VK_SUCCESS) {\
		std::string error = "Vulkan-Error: function ---> "; error += #x; error += " failed with error code " + vk::GetVkResultString(___vk_i_result____);\
		log_error(error.c_str(), __FILE__, __LINE__);\
	}\
}
#define vkcheck_break(x)\
{\
	VkResult ___vk_i_result____ = x;\
	if(___vk_i_result____  != VK_SUCCESS) {\
		std::string error = "Vulkan-Error: function ---> "; error += #x; error += " failed with error code " + vk::GetVkResultString(___vk_i_result____);\
		logerrors_break(error);\
	}\
}
#else
#define vkcheck(x) x
#define vkcheck_break(x) x
#endif

namespace vk {

	struct GraphicsCard
	{
		char name[256];
		uint64_t memorySize;
		VkPhysicalDeviceType deviceType;
		VkPhysicalDeviceLimits deviceLimits;
		std::vector<VkQueueFamilyProperties> QueueFamilies;
		VkPhysicalDevice handle;

		// This is to get rid of annoying compiler warnings
		GraphicsCard() {
			name[0] = 0;
			memorySize = 0;
			deviceType = VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM;
			deviceLimits = {};
			handle = NULL;
		}

	};

	class Swapchain;

	struct _tagVkContext
	{
		char m_ApiType = 0; // 0 for vulkan, 1 for opengl
		_FrameInformation* FrameInfo;
		VkInstance instance;
		PlatformWindow* window;
		VkDebugUtilsMessengerEXT debugMessenger;
		GraphicsCard card;
		VkDevice defaultDevice;
		VkQueue defaultQueue;
		uint32_t defaultQueueFamilyIndex;
		TextureSamples m_MaxMSAASamples;
		bool debugEnabled;
		void* m_memory_context;
		VkAlloc::CONTEXT m_future_memory_context;
		VkAllocationCallbacks* m_allocation_callback;
	} typedef* VkContext;

	VkContext Gfx_CreateContext(PlatformWindow* Window, bool EnableDebug, bool ForceIntegeratedGPU,
		const std::vector<const char*>& Layers, const std::vector<const char*>& LayerExtensions,
		std::vector<const char*> LogicalDeviceExtensions, VkPhysicalDeviceFeatures2 GraphicsCardFeatures,
		uint32_t VulkanAPIVersion = VK_API_VERSION_1_2);
	void Gfx_DestroyContext(VkContext context);

	std::vector<GraphicsCard> Gfx_GetAllGraphicsCards(VkContext context);
	GraphicsCard Gfx_GetDefaultCard(VkContext context, bool ForceIntegratedGPU);

	VkDevice Gfx_CreateDevice
	(GraphicsCard& card, std::vector<const char*> enabledExtensions, int* FamilyQueueIndex, VkQueueFlags queueFlags, int queueCount, VkPhysicalDeviceFeatures2 enabledFeatures);

	VkAttachmentDescription Gfx_InitImageAttachment(
		VkFormat imageFormat, VkImageLayout InitialLayout, VkImageLayout FinalLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
		VkAttachmentLoadOp loadStencilOp, VkAttachmentStoreOp storeStencilOp, VkSampleCountFlagBits samples);

	/*
		VkSubpassDescriptionFlags       flags;
		VkPipelineBindPoint             pipelineBindPoint;
		uint32_t                        inputAttachmentCount;
		const VkAttachmentReference*    pInputAttachments;
		uint32_t                        colorAttachmentCount;
		const VkAttachmentReference*    pColorAttachments;
		const VkAttachmentReference*    pResolveAttachments;
		const VkAttachmentReference*    pDepthStencilAttachment;
		uint32_t                        preserveAttachmentCount;
		const uint32_t*                 pPreserveAttachments;
	*/

	VkFence Gfx_CreateFence(VkContext context);
	VkSemaphore Gfx_CreateSemaphore(VkContext context, bool TimelineSemaphore);

	VkCommandPool Gfx_CreateCommandPool(VkContext context, bool memoryShortLived, bool enableIndividualReset, bool makeProtected = false);
	VkCommandBuffer Gfx_AllocCommandBuffer(VkContext context, VkCommandPool pool, bool primaryLevel);

	/*
	VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT specifies that each recording of the command buffer will only be submitted once, and the command buffer will be reset and recorded again between each submission.

	VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT specifies that a secondary command buffer is considered to be entirely inside a render pass. If this is a primary command buffer, then this bit is ignored.

	VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT specifies that a command buffer can be resubmitted to a queue while it is in the pending state, and recorded into multiple primary command buffers.
	*/
	void Gfx_StartCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage);

	/*************************************************** PIPELINE LAYOUT ***************************************************/

	/*
	VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT specifies that descriptor sets can return their individual allocations to the pool, i.e. all of vkAllocateDescriptorSets, vkFreeDescriptorSets, and vkResetDescriptorPool are allowed. Otherwise, descriptor sets allocated from the pool must not be individually freed back to the pool, i.e. only vkAllocateDescriptorSets and vkResetDescriptorPool are allowed.

	VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT specifies that descriptor sets allocated from this pool can include bindings with the VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT bit set. It is valid to allocate descriptor sets that have bindings that do not set the VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT bit from a pool that has VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT set.

	VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_VALVE specifies that this descriptor pool and the descriptor sets allocated from it reside entirely in host memory and cannot be bound. Descriptor sets allocated from this pool are partially exempt from the external synchronization requirement in vkUpdateDescriptorSetWithTemplateKHR and vkUpdateDescriptorSets. Descriptor sets and their descriptors can be updated concurrently in different threads, though the same descriptor must not be updated concurrently by two threads.
	*/
	VkDescriptorPool Gfx_CreateDescriptorPool(VkContext context, int maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes, VkDescriptorPoolCreateFlags flags = (VkDescriptorPoolCreateFlags)0);

	VkPipelineLayout Gfx_CreatePipelineLayout
	(VkContext context, const std::vector<VkDescriptorSetLayout>& setLayouts, const std::vector<VkPushConstantRange>& pushconstants);

	void Gfx_UpdateDescriptorSetBuffer(VkContext context, VkDescriptorSet set, VkBuffer buffer, int binding, VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, int descriptorCount = 1);
	void Gfx_UpdateDescriptorSetImage(VkContext context, VkDescriptorSet set, void* ImageView, IGPUTextureSampler sampler, int binding, VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, int descriptorCount = 1);

	void Gfx_SubmitCmdBuffers(VkQueue queue, std::vector<VkCommandBuffer> cmdBuffers, std::vector<VkSemaphore> waitSemaphores, std::vector<VkPipelineStageFlags> waitDstStageMask, std::vector<VkSemaphore> signalSemaphores, VkFence fence);

	// https://harrylovescode.gitbooks.io/vulkan-api/content/chap07/chap07.html with some modifications
	void Gfx_SetImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
		VkImageAspectFlags aspects,
		VkImageLayout oldLayout,
		VkImageLayout newLayout);

	size_t PadUniformBuffer(VkContext context, size_t struct_size);
	std::string GetVkResultString(VkResult result);

}