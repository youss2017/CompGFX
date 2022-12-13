#include "egxcommon.hpp"
#include <Utility/CppUtility.hpp>

egx::VulkanCoreInterface::~VulkanCoreInterface() {
	vkDeviceWaitIdle(Device);
	VkAlloc::DestroyContext(MemoryContext);
#ifdef _DEBUG
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(Instance, DebugMessenger, nullptr);
	}
#endif
	vkDestroyDevice(Device, nullptr);
	vkDestroyInstance(Instance, nullptr);
}
