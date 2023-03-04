#pragma once
#include "vulkinc.hpp"
#include <string>
#undef EGX_API
#ifdef BUILD_GRAPHICS_DLL
#if BUILD_GRAPHICS_DLL == 1
#define EGX_API _declspec(dllexport)
#else
#define EGX_API _declspec(dllimport)
#endif
#else
#define EGX_API
#endif
#define COMPGFX EGX_API
#include "memory/vulkan_memory_allocator.hpp"
#include "memory/egxref.hpp"
#include <cstdint>

namespace egx {

	struct Device {
		VkPhysicalDevice Id;
		size_t VideoRam;
		size_t SharedSystemRam;
		std::string VendorName;
		bool IsDedicated;
		VkPhysicalDeviceProperties Properties;
		bool CardHasDedicatedCompute = false;
		bool CardHasDedicatedTransfer = false;
	};

	struct VulkanCoreInterface {
		VkInstance Instance = nullptr;
		Device PhysicalDevice = {};
		VkDevice Device = nullptr;
		VkQueue Queue = nullptr;
		VkQueue DedicatedTransfer = nullptr;
		VkQueue DedicatedCompute = nullptr;
		int32_t GraphicsQueueFamilyIndex = -1;
		int32_t ComputeQueueFamilyIndex = -1;
		int32_t TransferQueueFamilyIndex = -1;
		VkAlloc::CONTEXT MemoryContext = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
		uint32_t MaxFramesInFlight = 1;
		uint32_t CurrentFrame = 0;
		size_t FrameCount = 0;
		VkPhysicalDeviceFeatures2 Features{};

		EGX_API VulkanCoreInterface() = default;
		EGX_API ~VulkanCoreInterface();

	};

	inline uint32_t MinAlignmentForUniform(ref<VulkanCoreInterface>& CoreInterface, uint32_t size) {
		uint32_t alignment = (uint32_t)CoreInterface->PhysicalDevice.Properties.limits.minUniformBufferOffsetAlignment;
		uint32_t x = size % alignment;
		if (x) {
			size += alignment - x;
		}
		return size;
	}

	inline uint32_t MinAlignmentForStorage(ref<VulkanCoreInterface>& CoreInterface, uint32_t size) {
		uint32_t alignment = (uint32_t)CoreInterface->PhysicalDevice.Properties.limits.minStorageBufferOffsetAlignment;
		uint32_t x = size % alignment;
		if (x) {
			size += alignment - x;
		}		return size;
	}

}
