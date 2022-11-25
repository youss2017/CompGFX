#pragma once
#include "../core/egxcommon.hpp"
#include <string>

// For More info check https://www.saschawillems.de/blog/2016/05/28/tutorial-on-using-vulkans-vk_ext_debug_marker-with-renderdoc/

namespace egx
{
#ifdef _DEBUG
	template<typename T>
	VkResult SetObjectName(const ref<VulkanCoreInterface>& CoreInterface, T nativeObject, VkDebugReportObjectTypeEXT type, const std::string& name)
	{
		static PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = nullptr;
		if (!pfnDebugMarkerSetObjectName) {
			pfnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(CoreInterface->Device, "vkDebugMarkerSetObjectNameEXT");
		}
		// may not be supported
		if (pfnDebugMarkerSetObjectName) {
			VkDebugMarkerObjectNameInfoEXT ext{ VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT };
			ext.objectType = type;
			ext.object = uint64_t(nativeObject);
			ext.pObjectName = name.c_str();
			return pfnDebugMarkerSetObjectName(CoreInterface->Device, &ext);
		}
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
#else
#define SetObjectName(...)
#endif

}
