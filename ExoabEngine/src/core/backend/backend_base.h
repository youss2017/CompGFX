#pragma once
#include "../../utils/Profiling.hpp"
#include "../../utils/defines.h"
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <common.hpp>

typedef void* APIHandle;
typedef void *GraphicsContext, *GraphicsSwapchain, *OpaqueIdentifier;

struct FrameData {
	uint32_t m_FrameIndex;
	/*
		We wait on Present Semaphore to know when swapchain is ready.
		We signal render semaphore to tell when rendering is complete so that we can vkQueuePresent
	*/
	VkSemaphore m_PresentSemaphore, m_RenderSemaphore;
	/*
		Render fence is used to synchronize the GPU with CPU to work on the current frame.
	*/
	VkFence m_RenderFence;
	VkCommandPool m_pool;
	VkCommandBuffer m_cmd;
};

#define ToVKContext(x) ((vk::VkContext)x)
#define GetVKDevice(x) ((ToVKContext(x))->defaultDevice)

constexpr int gFrameOverlapCount = 3;
