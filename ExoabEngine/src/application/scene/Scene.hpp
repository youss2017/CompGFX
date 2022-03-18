#pragma once
#include <vulkan/vulkan_core.h>
#include <backend/backend_base.h>
#include <backend/VulkanLoader.h>

namespace Application {

	class Scene {
		
	public:
		Scene(VkDevice device, bool primary) : mDevice(device) {
			for (int i = 0; i < gFrameOverlapCount; i++) {
				VkCommandPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
				vkCreateCommandPool(device, &createInfo, nullptr, &mPools[i]);
				VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
				allocInfo.commandBufferCount = 1;
				allocInfo.commandPool = mPools[i];
				allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
				vkAllocateCommandBuffers(device, &allocInfo, &mCmds[i]);
			}
		}
		
		Scene(const Scene& other) = delete;
		Scene(const Scene&& other) = delete;

		virtual void Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) = 0;
		virtual VkCommandBuffer Frame(uint32_t FrameIndex) = 0;

	protected:
		void Super_Scene() {
			for (int i = 0; i < gFrameOverlapCount; i++) {
				vkDestroyCommandPool(mDevice, mPools[i], nullptr);
			}
		}

	protected:
		VkDevice mDevice;
		VkCommandPool mPools[gFrameOverlapCount];
		VkCommandBuffer mCmds[gFrameOverlapCount];
	};

}
