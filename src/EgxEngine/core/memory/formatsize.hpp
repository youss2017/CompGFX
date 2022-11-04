#pragma once
#include <vulkan/vulkan_core.h>
#include <string>

namespace egx {
	namespace _internal {

		long FormatByteCount(VkFormat format);
		std::string DebugVkFormatToString(VkFormat format);
	}
}
