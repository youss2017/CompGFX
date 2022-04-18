#pragma once
#include "../../window/PlatformWindow.hpp"
#include "backend_base.h"
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

class FramebufferAttachment;
struct GPUTexture2D_2;

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

	struct _VkContext
	{
		char m_ApiType = 0; // 0 for vulkan, 1 for opengl
		VkInstance instance;
		PlatformWindow* window;
		VkDebugUtilsMessengerEXT debugMessenger;
		GraphicsCard card;
		VkDevice defaultDevice;
		VkQueue defaultQueue;
		uint32_t defaultQueueFamilyIndex;
		uint32_t m_MaxMSAASamples;
		bool debugEnabled;
		VkAlloc::CONTEXT m_future_memory_context;
		uint32_t mFrameIndex = 0;
	} typedef* VkContext;

	VkContext Gfx_CreateContext(PlatformWindow* Window, bool EnableDebug, bool ForceIntegeratedGPU,
		const std::vector<const char*>& Layers, const std::vector<const char*>& LayerExtensions,
		std::vector<const char*> LogicalDeviceExtensions, VkPhysicalDeviceFeatures2 GraphicsCardFeatures,
		uint32_t VulkanAPIVersion = VK_API_VERSION_1_2);
	void Gfx_DestroyContext(VkContext context);

	std::vector<GraphicsCard> Gfx_GetAllGraphicsCards(VkContext context);
	GraphicsCard Gfx_GetDefaultCard(VkContext context, bool ForceIntegratedGPU, VkPhysicalDeviceFeatures2 requiredFeatures);

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

	VkFence Gfx_CreateFence(VkContext context, bool signaled);
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

	void Gfx_SubmitCmdBuffers(VkQueue queue, std::vector<VkCommandBuffer> cmdBuffers, std::vector<VkSemaphore> waitSemaphores, std::vector<VkPipelineStageFlags> waitDstStageMask, std::vector<VkSemaphore> signalSemaphores, VkFence fence);

	VkSampler Gfx_CreateSampler(VkContext context,
		VkFilter magFilter = VK_FILTER_LINEAR,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VkSamplerAddressMode u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkSamplerAddressMode v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkSamplerAddressMode w = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		float mipLodBias = 0.0,
		VkBool32 anisotropyEnable = VK_TRUE,
		float maxAnisotropy = 16.0,
		float minLod = 0.0,
		float maxLod = 1000.0, 
		VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	// https://harrylovescode.gitbooks.io/vulkan-api/content/chap07/chap07.html with some modifications
	void Gfx_SetImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
		VkImageAspectFlags aspects,
		VkImageLayout oldLayout,
		VkImageLayout newLayout);

	VkQueryPool Gfx_CreateQueryPool(VkContext context, VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics);

	struct SingleUseCmdBuffer {
		VkDevice device;
		VkQueue queue;
		VkCommandPool pool;
		VkFence fence;
		VkCommandBuffer cmd;
	};

	SingleUseCmdBuffer Gfx_CreateSingleUseCmdBuffer(VkContext context);
	void Gfx_SubmitSingleUseCmdBufferAndDestroy(SingleUseCmdBuffer& buffer);

	VkBufferMemoryBarrier  Gfx_BufferMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkBuffer buffer);

	size_t PadUniformBuffer(VkContext context, size_t struct_size);
	std::string GetVkResultString(VkResult result);

	/*
	Since were using dynamic rendering (no render pass) we must transistion textures into "final layout" in the first use
	*/

	void Framebuffer_TransistionAttachment(VkCommandBuffer cmd, FramebufferAttachment* attachment, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkAccessFlags srcAccess = VK_ACCESS_NONE,
		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

	void Framebuffer_TransistionImage(VkCommandBuffer cmd, GPUTexture2D_2* attachment, VkImageAspectFlags aspect, uint32_t frameIndex, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkAccessFlags srcAccess = VK_ACCESS_NONE,
		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

	void Framebuffer_TransistionImage(VkCommandBuffer cmd, FramebufferAttachment* attachment, uint32_t frameIndex, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkAccessFlags srcAccess = VK_ACCESS_NONE,
		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

	void Gfx_InsertDebugLabel(VkCommandBuffer cmd, int FrameIndex, const std::string& debugLabel, float r = 0.0, float g = 0.0, float b = 0.0);

}