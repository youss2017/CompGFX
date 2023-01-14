#include "cmd.hpp"
#include <Utility/CppUtility.hpp>
#include "../utils/objectdebugger.hpp"

namespace egx
{
	// CommandBuffer does not need to keep ref to VulkanCoreInterface
	// because _cmd_pool already has reference and it will only be destroyed if
	// _cmd_pool (of all threads) are destroyed.
	namespace internal
	{
		static thread_local egx::ref<egx::CommandPool> _cmd_pool = nullptr;
	}

	CommandBuffer::CommandBuffer(const ref<VulkanCoreInterface>& CoreInterface)
	{
		DelayInitialize(CoreInterface);
	}

	void CommandBuffer::DelayInitialize(const ref<VulkanCoreInterface>& CoreInterface)
	{
		if (!internal::_cmd_pool.IsValidRef())
		{
			internal::_cmd_pool = CreateCommandPool(CoreInterface, true, false, true, false);
#ifdef _DEBUG
			LOG(INFO, "[Vulkan] Command Pool Initalized, Ref {{0x{0:%p}}}", internal::_cmd_pool.base);
#endif
		}
		_cmd = internal::_cmd_pool->AllocateBufferFrameFlightMode(true);
		_cmd_static_init.resize(CoreInterface->MaxFramesInFlight, false);
		DelayInitalizeFF(CoreInterface);
		_Queue = CoreInterface->Queue;
	}

	void CommandBuffer::SetDebugName(const ref<VulkanCoreInterface>& CoreInterface, const std::string& Name)
	{
		for (size_t i = 0; i < _cmd.size(); i++)
		{
			SetObjectName(CoreInterface, _cmd[i], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, cpp::Format("{0} [{1}]", Name, i));
		}
	}

	CommandBuffer::CommandBuffer(CommandBuffer&& move) noexcept
	{
		this->_cmd = std::move(move._cmd);
		this->_cmd_static_init = std::move(move._cmd_static_init);
		move._cmd.resize(0);
	}

	CommandBuffer::~CommandBuffer() noexcept
	{
		if (_cmd.size() > 0 && internal::_cmd_pool.IsValidRef())
			internal::_cmd_pool->FreeCommandBuffers(_cmd);
	}

	const VkCommandBuffer CommandBuffer::GetBuffer()
	{
		assert(_cmd.size() > 0);
		uint32_t current_frame = GetCurrentFrame();
		VkCommandBuffer cmd = _cmd[current_frame];
		if (_last_frame != current_frame || *_ptr_frame_count != _last_frame_count)
		{
			vkResetCommandBuffer(cmd, 0);
			StartCommandBuffer(cmd, 0);
			_last_frame = current_frame;
			_last_frame_count = *_ptr_frame_count;
			_cmd_static_init[current_frame] = false;
		}
		return cmd;
	}

	const VkCommandBuffer& CommandBuffer::GetReadonlyBuffer() const
	{
		assert(_cmd_static_init[GetCurrentFrame()] && "You must record static buffer");
		return _cmd[GetCurrentFrame()];
	}

	void CommandBuffer::Submit(VkFence fence)
	{
		Submit(_Queue, { GetReadonlyBuffer() }, {}, {}, {}, fence, false);
	}

	void CommandBuffer::Submit(const ref<Fence> fence)
	{
		Submit(_Queue, { GetReadonlyBuffer() }, {}, {}, {}, fence->GetFence(), false);
	}

	void CommandBuffer::Finalize()
	{
		assert(_cmd.size() > 0);
		uint32_t current_frame = GetCurrentFrame();
		VkCommandBuffer cmd = _cmd[current_frame];
		vkEndCommandBuffer(cmd);
		_cmd_static_init[current_frame] = true;
		if (_cmd.size() == 1)
			_last_frame = UINT32_MAX;
	}

	ref<CommandBuffer> CommandBuffer::CreateSingleBuffer(const ref<VulkanCoreInterface>& CoreInterface)
	{
		ref<CommandBuffer> cmd = new CommandBuffer;
		if (!internal::_cmd_pool.IsValidRef())
		{
			internal::_cmd_pool = CreateCommandPool(CoreInterface, true, false, true, false);
#ifdef _DEBUG
			LOG(INFO, "[Vulkan] Command Pool Initalized, Ref {{{0}}}", internal::_cmd_pool.base);
#endif
		}
		cmd->_cmd.push_back(internal::_cmd_pool->AllocateBuffer(true));
		cmd->_cmd_static_init.resize(1, false);
		cmd->DelayInitalizeFF(CoreInterface, true);
		return cmd;
	}

	void CommandBuffer::Submit(VkQueue Queue,
		const std::initializer_list<VkCommandBuffer>& CmdBuffers,
		const std::initializer_list<VkSemaphore>& WaitSemaphores,
		const std::initializer_list<VkPipelineStageFlags>& WaitDstStageMask,
		const std::initializer_list<VkSemaphore>& SignalSemaphores,
		VkFence SignalFence,
		bool Block)
	{
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.waitSemaphoreCount = (uint32_t)WaitSemaphores.size();
		submitInfo.pWaitSemaphores = WaitSemaphores.begin();
		submitInfo.pWaitDstStageMask = WaitDstStageMask.begin();
		submitInfo.commandBufferCount = (uint32_t)CmdBuffers.size();
		submitInfo.pCommandBuffers = CmdBuffers.begin();
		submitInfo.signalSemaphoreCount = (uint32_t)SignalSemaphores.size();
		submitInfo.pSignalSemaphores = SignalSemaphores.begin();
		vkQueueSubmit(Queue, 1, &submitInfo, SignalFence);
		if (Block)
			vkQueueWaitIdle(Queue);
	}

	void CommandBuffer::Submit(const ref<VulkanCoreInterface>& CoreInterface,
		const std::initializer_list<VkCommandBuffer>& CmdBuffers,
		const std::initializer_list<VkSemaphore>& WaitSemaphores,
		const std::initializer_list<VkPipelineStageFlags>& WaitDstStageMask,
		const std::initializer_list<VkSemaphore>& SignalSemaphores,
		VkFence SignalFence,
		bool Block)
	{
		Submit(CoreInterface->Queue, CmdBuffers, WaitSemaphores, WaitDstStageMask, SignalSemaphores, SignalFence, Block);
	}

	void CommandBuffer::Submit(VkQueue Queue,
		const std::vector<VkCommandBuffer>& CmdBuffers,
		const std::vector<VkSemaphore>& WaitSemaphores,
		const std::vector<VkPipelineStageFlags>& WaitDstStageMask,
		const std::vector<VkSemaphore>& SignalSemaphores,
		VkFence SignalFence,
		bool Block)
	{
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.waitSemaphoreCount = (uint32_t)WaitSemaphores.size();
		submitInfo.pWaitSemaphores = WaitSemaphores.data();
		submitInfo.pWaitDstStageMask = WaitDstStageMask.data();
		submitInfo.commandBufferCount = (uint32_t)CmdBuffers.size();
		submitInfo.pCommandBuffers = CmdBuffers.data();
		submitInfo.signalSemaphoreCount = (uint32_t)SignalSemaphores.size();
		submitInfo.pSignalSemaphores = SignalSemaphores.data();
		vkQueueSubmit(Queue, 1, &submitInfo, SignalFence);
		if (Block)
			vkQueueWaitIdle(Queue);
	}

	void CommandBuffer::Submit(const ref<VulkanCoreInterface>& CoreInterface,
		const std::vector<VkCommandBuffer>& CmdBuffers,
		const std::vector<VkSemaphore>& WaitSemaphores,
		const std::vector<VkPipelineStageFlags>& WaitDstStageMask,
		const std::vector<VkSemaphore>& SignalSemaphores,
		VkFence SignalFence,
		bool Block)
	{
		Submit(CoreInterface->Queue, CmdBuffers, WaitSemaphores, WaitDstStageMask, SignalSemaphores, SignalFence, Block);
	}

	std::vector<VkSemaphore> CommandBuffer::GetNativeSemaphoreList(const std::vector<ref<Semaphore>>& semaphores)
	{
		std::vector<VkSemaphore> result;
		for (auto& item : semaphores) {
			result.push_back(item->GetSemaphore());
		}
		return result;
	}

	void CommandBuffer::StartCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage)
	{
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = usage;
		vkBeginCommandBuffer(buffer, &beginInfo);
	}

}