#pragma once
#include "../../utils/Profiling.hpp"
#include "../../utils/defines.h"
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <common.hpp>
#include "memory/vulkan_memory_allocator.hpp"

#undef GRAPHICS_API

#ifdef BUILD_GRAPHICS_DLL
#if BUILD_GRAPHICS_DLL == 1
#define GRAPHICS_API __declspec(dllexport)
#else
#define GRAPHICS_API __declspec(dllimport)
#endif
#else
#define GRAPHICS_API
#endif

class PlatformWindow;

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
}

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

constexpr int gFrameOverlapCount = 3;
