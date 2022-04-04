#pragma once
#include "VkGraphicsCard.hpp"
#include "Globals.hpp"

class CommandPool {

public:
	CommandPool() {
		for (int i = 0; i < gFrameOverlapCount; i++) {
			mPools[i] = vk::Gfx_CreateCommandPool(Global::Context, false, false);
		}
	}

	~CommandPool() {
		for (int i = 0; i < gFrameOverlapCount; i++) {
			if (mPools[i])
				vkDestroyCommandPool(Global::Context->defaultDevice, mPools[i], nullptr);
		}
	}
	
	CommandPool(const CommandPool& other) = delete;
	CommandPool(const CommandPool&& other) = delete;

	inline void Reset() {
		vkResetCommandPool(Global::Context->defaultDevice, *this, 0);
	}

	inline void Reset(uint32_t FrameIndex) {
		vkResetCommandPool(Global::Context->defaultDevice, mPools[FrameIndex], 0);
	}

	inline operator VkCommandPool() {
		return mPools[Global::Context->mFrameIndex];
	}

	VkCommandPool mPools[gFrameOverlapCount];

};

class CommandBuffer {
public:
	CommandBuffer(CommandPool* pool, bool primary = true) : mPool(pool) {
		for (int i = 0; i < gFrameOverlapCount; i++) {
			VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
			allocateInfo.commandPool = pool->mPools[i];
			allocateInfo.commandBufferCount = 1;
			allocateInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			vkAllocateCommandBuffers(Global::Context->defaultDevice, &allocateInfo, &mCmds[i]);
		}
	}
	
	~CommandBuffer() {
		if (*mPool != nullptr) {
			for (int i = 0; i < gFrameOverlapCount; i++) {
				vkFreeCommandBuffers(Global::Context->defaultDevice, mPool->mPools[i], 1, &mCmds[i]);
			}
		}
	}

	CommandBuffer(const CommandBuffer& other) = delete;
	CommandBuffer(const CommandBuffer&& other) = delete;

	inline operator VkCommandBuffer() {
		return mCmds[Global::Context->mFrameIndex];
	}

	VkCommandBuffer mCmds[gFrameOverlapCount];
	CommandPool* mPool;
};
