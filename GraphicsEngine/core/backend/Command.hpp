#pragma once
#include "VkGraphicsCard.hpp"

class CommandPool {

public:
	CommandPool(vk::VkContext context) : mContext(context) {
		for (int i = 0; i < gFrameOverlapCount; i++) {
			mPools[i] = vk::Gfx_CreateCommandPool(false, false);
		}
	}

	~CommandPool() {
		for (int i = 0; i < gFrameOverlapCount; i++) {
			if (mPools[i])
				vkDestroyCommandPool(mContext->defaultDevice, mPools[i], nullptr);
		}
	}
	
	CommandPool(const CommandPool& other) = delete;
	CommandPool(const CommandPool&& other) = delete;

	inline void Reset() {
		vkResetCommandPool(mContext->defaultDevice, *this, 0);
	}

	inline void Reset(uint32_t FrameIndex) {
		vkResetCommandPool(mContext->defaultDevice, mPools[FrameIndex], 0);
	}

	inline operator VkCommandPool() {
		return mPools[mContext->mFrameIndex];
	}

	VkCommandPool mPools[gFrameOverlapCount];
	vk::VkContext mContext;

};

class CommandBuffer {
public:
	CommandBuffer(CommandPool* pool, bool primary = true) : mPool(pool) {
		for (int i = 0; i < gFrameOverlapCount; i++) {
			VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
			allocateInfo.commandPool = pool->mPools[i];
			allocateInfo.commandBufferCount = 1;
			allocateInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			vkAllocateCommandBuffers(pool->mContext->defaultDevice, &allocateInfo, &mCmds[i]);
		}
	}
	
	~CommandBuffer() {
		if (*mPool != nullptr) {
			for (int i = 0; i < gFrameOverlapCount; i++) {
				vkFreeCommandBuffers(mPool->mContext->defaultDevice, mPool->mPools[i], 1, &mCmds[i]);
			}
		}
	}

	CommandBuffer(const CommandBuffer& other) = delete;
	CommandBuffer(const CommandBuffer&& other) = delete;

	inline operator VkCommandBuffer() {
		return mCmds[mPool->mContext->mFrameIndex];
	}

	VkCommandBuffer mCmds[gFrameOverlapCount];
	CommandPool* mPool;
};
