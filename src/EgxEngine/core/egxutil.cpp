#include "egxutil.hpp"
#include <string>

void egx::InsertDebugLabel(ref<VulkanCoreInterface>& CoreInterface, VkCommandBuffer cmd, uint32_t FrameIndex, std::string_view DebugLabelText, float r, float g, float b)
{
	using namespace std;
	std::string labelName = "[" + std::to_string(FrameIndex) + "]"s + DebugLabelText.data();
	VkDebugUtilsLabelEXT debugLabel{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	debugLabel.pLabelName = labelName.c_str();
	float debugColor[4] = { r, g, b, 1.0 };
	memcpy(&debugLabel.color[0], &debugColor[0], sizeof(float) * 4);
	static PFN_vkCmdBeginDebugUtilsLabelEXT beginUtilsLabelEXT = nullptr;
	if (!beginUtilsLabelEXT) {
		beginUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(CoreInterface->Device, "vkCmdBeginDebugUtilsLabelEXT");
	}
	beginUtilsLabelEXT(cmd, &debugLabel);
}

void EGX_API egx::EndDebugLabel(ref<VulkanCoreInterface>& CoreInterface, VkCommandBuffer cmd)
{
	static PFN_vkCmdEndDebugUtilsLabelEXT endUtilsLabelEXT = nullptr;
	if (!endUtilsLabelEXT) {
		endUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(CoreInterface->Device, "vkCmdEndDebugUtilsLabelEXT");
	}
	endUtilsLabelEXT(cmd);
}

std::string egx::VkResultToString(VkResult result) {
	static std::map<VkResult, std::string> mapping;
	if (mapping.size() == 0) {
		mapping.insert({VK_SUCCESS, "VK_SUCCESS"});
		mapping.insert({VK_NOT_READY, "VK_NOT_READY"});
		mapping.insert({VK_TIMEOUT, "VK_TIMEOUT"});
		mapping.insert({VK_EVENT_SET, "VK_EVENT_SET"});
		mapping.insert({VK_EVENT_RESET, "VK_EVENT_RESET"});
		mapping.insert({VK_INCOMPLETE, "VK_INCOMPLETE"});
		mapping.insert({VK_ERROR_OUT_OF_HOST_MEMORY, "VK_ERROR_OUT_OF_HOST_MEMORY"});
		mapping.insert({VK_ERROR_OUT_OF_DEVICE_MEMORY, "VK_ERROR_OUT_OF_DEVICE_MEMORY"});
		mapping.insert({VK_ERROR_INITIALIZATION_FAILED, "VK_ERROR_INITIALIZATION_FAILED"});
		mapping.insert({VK_ERROR_DEVICE_LOST, "VK_ERROR_DEVICE_LOST"});
		mapping.insert({VK_ERROR_MEMORY_MAP_FAILED, "VK_ERROR_MEMORY_MAP_FAILED"});
		mapping.insert({VK_ERROR_LAYER_NOT_PRESENT, "VK_ERROR_LAYER_NOT_PRESENT"});
		mapping.insert({VK_ERROR_EXTENSION_NOT_PRESENT, "VK_ERROR_EXTENSION_NOT_PRESENT"});
		mapping.insert({VK_ERROR_FEATURE_NOT_PRESENT, "VK_ERROR_FEATURE_NOT_PRESENT"});
		mapping.insert({VK_ERROR_INCOMPATIBLE_DRIVER, "VK_ERROR_INCOMPATIBLE_DRIVER"});
		mapping.insert({VK_ERROR_TOO_MANY_OBJECTS, "VK_ERROR_TOO_MANY_OBJECTS"});
		mapping.insert({VK_ERROR_FORMAT_NOT_SUPPORTED, "VK_ERROR_FORMAT_NOT_SUPPORTED"});
		mapping.insert({VK_ERROR_FRAGMENTED_POOL, "VK_ERROR_FRAGMENTED_POOL"});
		mapping.insert({VK_ERROR_UNKNOWN, "VK_ERROR_UNKNOWN"});
		mapping.insert({VK_ERROR_OUT_OF_POOL_MEMORY, "VK_ERROR_OUT_OF_POOL_MEMORY"});
		mapping.insert({VK_ERROR_INVALID_EXTERNAL_HANDLE, "VK_ERROR_INVALID_EXTERNAL_HANDLE"});
		mapping.insert({VK_ERROR_FRAGMENTATION, "VK_ERROR_FRAGMENTATION"});
		mapping.insert({VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"});
		mapping.insert({VK_PIPELINE_COMPILE_REQUIRED, "VK_PIPELINE_COMPILE_REQUIRED"});
		mapping.insert({VK_ERROR_SURFACE_LOST_KHR, "VK_ERROR_SURFACE_LOST_KHR"});
		mapping.insert({VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"});
		mapping.insert({VK_SUBOPTIMAL_KHR, "VK_SUBOPTIMAL_KHR"});
		mapping.insert({VK_ERROR_OUT_OF_DATE_KHR, "VK_ERROR_OUT_OF_DATE_KHR"});
		mapping.insert({VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"});
		mapping.insert({VK_ERROR_VALIDATION_FAILED_EXT, "VK_ERROR_VALIDATION_FAILED_EXT"});
		mapping.insert({VK_ERROR_INVALID_SHADER_NV, "VK_ERROR_INVALID_SHADER_NV"});
		mapping.insert({VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"});
		mapping.insert({VK_ERROR_NOT_PERMITTED_KHR, "VK_ERROR_NOT_PERMITTED_KHR"});
		mapping.insert({VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"});
		mapping.insert({VK_THREAD_IDLE_KHR, "VK_THREAD_IDLE_KHR"});
		mapping.insert({VK_THREAD_DONE_KHR, "VK_THREAD_DONE_KHR"});
		mapping.insert({VK_OPERATION_DEFERRED_KHR, "VK_OPERATION_DEFERRED_KHR"});
		mapping.insert({VK_OPERATION_NOT_DEFERRED_KHR, "VK_OPERATION_NOT_DEFERRED_KHR"});
		mapping.insert({VK_ERROR_COMPRESSION_EXHAUSTED_EXT, "VK_ERROR_COMPRESSION_EXHAUSTED_EXT"});
		mapping.insert({VK_ERROR_OUT_OF_POOL_MEMORY_KHR, "VK_ERROR_OUT_OF_POOL_MEMORY_KHR"});
		mapping.insert({VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR, "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR"});
		mapping.insert({VK_ERROR_FRAGMENTATION_EXT, "VK_ERROR_FRAGMENTATION_EXT"});
		mapping.insert({VK_ERROR_NOT_PERMITTED_EXT, "VK_ERROR_NOT_PERMITTED_EXT"});
		mapping.insert({VK_ERROR_INVALID_DEVICE_ADDRESS_EXT, "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT"});
		mapping.insert({VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR"});
		mapping.insert({VK_PIPELINE_COMPILE_REQUIRED_EXT, "VK_PIPELINE_COMPILE_REQUIRED_EXT"});
		mapping.insert({VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT, "VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT"});
		mapping.insert({VK_RESULT_MAX_ENUM, "VK_RESULT_MAX_ENUM"});
	}
	return mapping[result];
}
