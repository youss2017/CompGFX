#pragma once
#include <vulkan/vulkan_core.h>
#include <backend/backend_base.h>
#include <backend/VulkanLoader.h>
#include <backend/Command.hpp>
#include "Globals.hpp"

namespace Application {

	class Pass;

	struct PassPrepare {
		Pass* self;
		uint32_t nFrameIndex;
		// pOutCmd is written to after Prepare();
		VkCommandBuffer pOutCmd;
	};

	class Pass {
		
	public:
		Pass(VkDevice device, bool primary) : mDevice(device) {
			mCmdPool = new CommandPool(vk::Gfx_GetContext());
			mCmd = new CommandBuffer(mCmdPool, primary);
			pPrepare = new PassPrepare();
			pPrepare->self = this;
		}
		
		Pass(const Pass& other) = delete;
		Pass(const Pass&& other) = delete;

		inline PassPrepare* GetPassPrepare(uint32_t frameIndex) {
			pPrepare->nFrameIndex = frameIndex;
			return pPrepare;
		}

		virtual void ReloadShaders() = 0;
		// This function is called by Launch()
		virtual VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) = 0;

		// for multithreading access, pArgs is a pointer to PassPrepare
		// use GetPassPrepare and fillout frameindex, use pOutCmd in PassPrepare structure
		// it is the VkCommandBuffer
		static void Launch(void* pArgs);

	protected:
		virtual void RecordCommands(uint32_t FrameIndex) = 0;
		void Super_Pass_Destroy() {
			delete mCmd;
			delete mCmdPool;
			delete pPrepare;
		}


	protected:
		VkDevice mDevice;
		CommandPool* mCmdPool;
		CommandBuffer* mCmd;
		PassPrepare* pPrepare;
	};

}
