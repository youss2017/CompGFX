#pragma once
#include <vulkan/vulkan_core.h>
#include <backend/backend_base.h>
#include <backend/VulkanLoader.h>
#include <backend/Command.hpp>
#include "Globals.hpp"

namespace Application {

	class Pass {
		
	public:
		Pass(VkDevice device, bool primary) : mDevice(device) {
			mCmdPool = new CommandPool(Global::Context);
			mCmd = new CommandBuffer(mCmdPool, primary);
		}
		
		Pass(const Pass& other) = delete;
		Pass(const Pass&& other) = delete;

		virtual void ReloadShaders() = 0;
		virtual VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) = 0;

	protected:
		virtual void RecordCommands(uint32_t FrameIndex) = 0;
		void Super_Pass_Destroy() {
			delete mCmd;
			delete mCmdPool;
		}


	protected:
		VkDevice mDevice;
		CommandPool* mCmdPool;
		CommandBuffer* mCmd;
	};

}
