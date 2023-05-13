#pragma once
#include <vulkan/vulkan.hpp>
#include <string>

namespace egx {
	uint32_t FormatByteCount(VkFormat format);

	// Aspect Flags are parts of an image, for example:
	// Some images have a color section, and/or depth section, and/or stencil section
	// AspectFlags allow you to access only specific parts of the image
	// This is useful for clearing images, memory barriers, and image views 
	vk::ImageAspectFlags GetFormatAspectFlags(vk::Format format);
}
