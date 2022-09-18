#include "egxcommon.hpp"

egx::VulkanCoreInterface::~VulkanCoreInterface() {
	vkDeviceWaitIdle(Device);
	VkAlloc::DestroyContext(MemoryContext);
	vkDestroyDevice(Device, 0);
#ifdef _DEBUG
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(Instance, DebugMessenger, nullptr);
	}
#endif
	vkDestroyInstance(Instance, nullptr);
}
