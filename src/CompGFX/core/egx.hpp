#pragma once
#include "Utility/CppUtility.hpp"
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <memory>
#include <vector>

namespace egx
{
	struct PhysicalDeviceAndQueueFamilyInfo
	{
		char Name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
		vk::PhysicalDevice PhysicalDevice;
		vk::PhysicalDeviceType Type;
		std::vector<std::pair<uint32_t, std::vector<vk::QueueFlags>>> QueueFamilyInfo;
		VkPhysicalDeviceFeatures2 EnabledFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		bool SupportSwapchain = false;

		bool SupportExtension(const char* pExtensionName)
		{
			auto extensions = PhysicalDevice.enumerateDeviceExtensionProperties();
			for (auto& e : extensions)
			{
				if (strcmp(e.extensionName, pExtensionName) == 0)
					return true;
			}
			return false;
		}
	};

	class VulkanICDState;

	struct DeviceContext
	{
		PhysicalDeviceAndQueueFamilyInfo PhysicalDeviceQuery;
		bool SwapchainExtensionEnable = false;

		uint32_t FramesInFlight = 1;
		uint32_t CurrentFrame = 0;

		vk::Device Device;
		vk::Queue Queue;
		vk::Queue DedicatedTransfer;
		vk::Queue DedicatedCompute;
		int32_t GraphicsQueueFamilyIndex = -1;
		int32_t ComputeQueueFamilyIndex = -1;
		int32_t TransferQueueFamilyIndex = -1;

		VmaAllocator Allocator;

		cpp::Logger* pLogger;
		std::shared_ptr<VulkanICDState> ICDState;

		~DeviceContext();

		template <cpp::LogLevel level = cpp::LogLevel::INFO, typename... Arg>
		void Log(const std::string& message, Arg &&...args, int ln = __LINE__, const char* file = __FILENAME__) const
		{
			pLogger->print(level, file, ln, message, std::forward(args));
		}

		void NextFrame() {
			CurrentFrame++, CurrentFrame %= FramesInFlight;
		}
	};

	using DeviceCtx = std::shared_ptr<DeviceContext>;
	using VulkanICD = std::shared_ptr<VulkanICDState>;

	class VulkanICDState : public std::enable_shared_from_this<VulkanICDState>
	{
	public:
		static std::shared_ptr<VulkanICDState> Create(const std::string& pApplicationName,
			bool enableValidation,
			bool useGpuAssistedValidation,
			uint32_t vkApiVersion,
			cpp::Logger* pOptionalLogger,
			PFN_vkDebugUtilsMessengerCallbackEXT pCustomCallback);

		~VulkanICDState();

		std::vector<PhysicalDeviceAndQueueFamilyInfo> QueryGPGPUDevices();
		DeviceCtx CreateDevice(const PhysicalDeviceAndQueueFamilyInfo& deviceInformation, uint32_t max_frames_in_flight = 2);
		vk::Instance Instance() const { return m_Instance; }

	private:
		VulkanICDState(const std::string& pApplicationName,
			bool enableValidation,
			bool useGpuAssistedValidation,
			uint32_t vkApiVersion,
			cpp::Logger* pOptionalLogger,
			PFN_vkDebugUtilsMessengerCallbackEXT pCustomCallback);

	private:
		vk::Instance m_Instance;
		std::string m_ApplicationName;
		bool m_ValidationEnabled = false;
		bool m_GPUAssistedValidation = false;
		uint32_t m_ApiVersion = VK_API_VERSION_1_2;
		cpp::Logger* pOptionalLogger;
		PFN_vkDebugUtilsMessengerCallbackEXT pDebugCallback;
		vk::DebugUtilsMessengerEXT m_DebugMessengerEXT;
	};

	class IUniqueHandle {
	public:
		virtual std::unique_ptr<IUniqueHandle> MakeHandle() const = 0;
	};

	class ICallback {
	public:
		virtual void CallbackProtocol(void* pUserData) = 0;
	};

	class IUniqueWithCallback : public IUniqueHandle, public ICallback {};

	class ISynchronizationObject {
	public:
		/// <summary>
		/// Returns the semaphore for the current frame,
		/// that describes 
		/// </summary>
		/// <returns></returns>
		virtual std::pair<vk::Semaphore, vk::PipelineStageFlagBits> GetCurrentSignalSemaphore() const { return {}; };
		virtual bool IsUsingSignalSemaphore() const { return false; }
	};

}
