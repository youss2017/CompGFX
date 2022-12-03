#pragma once
#include <vulkan/vulkan_core.h>

namespace egx {
	VkBool32 GraphicsCardFeatureValidation_Check(VkPhysicalDevice device, VkPhysicalDeviceFeatures2 requiredFeatures);
}
