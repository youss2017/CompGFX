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

	class Swapchain;

	VkContext GRAPHICS_API Gfx_CreateContext(PlatformWindow* Window, bool EnableDebug, bool ForceIntegeratedGPU,
		const std::vector<const char*>& Layers, const std::vector<const char*>& LayerExtensions,
		std::vector<const char*> LogicalDeviceExtensions, VkPhysicalDeviceFeatures2 GraphicsCardFeatures,
		uint32_t VulkanAPIVersion = VK_API_VERSION_1_2);
	void GRAPHICS_API Gfx_DestroyContext(VkContext context);

	std::vector<GraphicsCard> GRAPHICS_API Gfx_GetAllGraphicsCards(VkContext context);
	GraphicsCard GRAPHICS_API Gfx_GetDefaultCard(VkContext context, bool ForceIntegratedGPU, VkPhysicalDeviceFeatures2 requiredFeatures);

	VkDevice GRAPHICS_API Gfx_CreateDevice
	(GraphicsCard& card, std::vector<const char*> enabledExtensions, int* FamilyQueueIndex, VkQueueFlags queueFlags, int queueCount, VkPhysicalDeviceFeatures2 enabledFeatures);

	VkFence GRAPHICS_API Gfx_CreateFence(VkContext context, bool signaled);
	VkSemaphore GRAPHICS_API Gfx_CreateSemaphore(VkContext context, bool TimelineSemaphore);

	VkCommandPool GRAPHICS_API Gfx_CreateCommandPool(VkContext context, bool memoryShortLived, bool enableIndividualReset, bool makeProtected = false);
	VkCommandBuffer GRAPHICS_API Gfx_AllocCommandBuffer(VkContext context, VkCommandPool pool, bool primaryLevel);

	/*
	VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT specifies that each recording of the command buffer will only be submitted once, and the command buffer will be reset and recorded again between each submission.

	VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT specifies that a secondary command buffer is considered to be entirely inside a render pass. If this is a primary command buffer, then this bit is ignored.

	VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT specifies that a command buffer can be resubmitted to a queue while it is in the pending state, and recorded into multiple primary command buffers.
	*/
	void GRAPHICS_API Gfx_StartCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage);

	/*************************************************** PIPELINE LAYOUT ***************************************************/

	/*
	VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT specifies that descriptor sets can return their individual allocations to the pool, i.e. all of vkAllocateDescriptorSets, vkFreeDescriptorSets, and vkResetDescriptorPool are allowed. Otherwise, descriptor sets allocated from the pool must not be individually freed back to the pool, i.e. only vkAllocateDescriptorSets and vkResetDescriptorPool are allowed.

	VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT specifies that descriptor sets allocated from this pool can include bindings with the VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT bit set. It is valid to allocate descriptor sets that have bindings that do not set the VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT bit from a pool that has VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT set.

	VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_VALVE specifies that this descriptor pool and the descriptor sets allocated from it reside entirely in host memory and cannot be bound. Descriptor sets allocated from this pool are partially exempt from the external synchronization requirement in vkUpdateDescriptorSetWithTemplateKHR and vkUpdateDescriptorSets. Descriptor sets and their descriptors can be updated concurrently in different threads, though the same descriptor must not be updated concurrently by two threads.
	*/
	VkDescriptorPool GRAPHICS_API Gfx_CreateDescriptorPool(VkContext context, int maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes, VkDescriptorPoolCreateFlags flags = (VkDescriptorPoolCreateFlags)0);

	void GRAPHICS_API Gfx_SubmitCmdBuffers(VkQueue queue, std::vector<VkCommandBuffer> cmdBuffers, std::vector<VkSemaphore> waitSemaphores, std::vector<VkPipelineStageFlags> waitDstStageMask, std::vector<VkSemaphore> signalSemaphores, VkFence fence);

	VkSampler GRAPHICS_API Gfx_CreateSampler(VkContext context,
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

	VkQueryPool GRAPHICS_API Gfx_CreateQueryPool(VkContext context, VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics);

	struct SingleUseCmdBuffer {
		VkDevice device;
		VkQueue queue;
		VkCommandPool pool;
		VkFence fence;
		VkCommandBuffer cmd;
	};

	SingleUseCmdBuffer GRAPHICS_API Gfx_CreateSingleUseCmdBuffer(VkContext context);
	void GRAPHICS_API Gfx_SubmitSingleUseCmdBufferAndDestroy(SingleUseCmdBuffer& buffer);

	VkBufferMemoryBarrier GRAPHICS_API Gfx_BufferMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkBuffer buffer);

	std::string GRAPHICS_API GetVkResultString(VkResult result);

	/*
	Since were using dynamic rendering (no render pass) we must transistion textures into "final layout" in the first use
	*/

	void GRAPHICS_API Framebuffer_TransistionAttachment(VkCommandBuffer cmd, FramebufferAttachment* attachment, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkAccessFlags srcAccess = VK_ACCESS_NONE,
		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

	void GRAPHICS_API Framebuffer_TransistionImage(VkCommandBuffer cmd, GPUTexture2D_2* attachment, VkImageAspectFlags aspect, uint32_t frameIndex, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkAccessFlags srcAccess = VK_ACCESS_NONE,
		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

	void GRAPHICS_API Framebuffer_TransistionImage(VkCommandBuffer cmd, FramebufferAttachment* attachment, uint32_t frameIndex, VkAccessFlags dstAccess, VkImageLayout newLayout,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkAccessFlags srcAccess = VK_ACCESS_NONE,
		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

	void GRAPHICS_API Gfx_InsertDebugLabel(VkDevice device, VkCommandBuffer cmd, int FrameIndex, const std::string& debugLabel, float r = 0.0, float g = 0.0, float b = 0.0);
	void GRAPHICS_API Gfx_EndDebugLabel(VkDevice device, VkCommandBuffer cmd);

}