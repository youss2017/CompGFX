#pragma once
#include <vulkan/vulkan_core.h>

namespace egx {
	VkBool32 ValidateFeatures(VkPhysicalDevice device, VkPhysicalDeviceFeatures2 requiredFeatures);
}
