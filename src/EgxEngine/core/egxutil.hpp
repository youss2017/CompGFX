#pragma once
#include "egxcommon.hpp"
#include <vulkan/vulkan_core.h>
#include <vector>
#include <string_view>
#include <string>
#include <imgui/imgui.h>

#ifdef _DEBUG
#define DInsertDebugLabel(RefCoreInterface, cmd, FrameIndex, DebugLabelText, ColR, ColG, ColB) egx::InsertDebugLabel(RefCoreInterface, cmd, FrameIndex, DebugLabelText, ColR, ColG, ColB)
#define DEndDebugLabel(RefCoreInterface, cmd) egx::EndDebugLabel(RefCoreInterface, cmd)
#else
#define DInsertDebugLabel(RefCoreInterface, cmd, FrameIndex, DebugLabelText, ColR, ColG, ColB)
#define DEndDebugLabel(RefCoreInterface, cmd)
#endif

namespace egx {

	class Fence {

	public:
		inline Fence(const ref<VulkanCoreInterface>& CoreInterface, bool CreateSignaled, bool FrameFlightMode) : _core(CoreInterface) {
			VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
			createInfo.flags = CreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
			if (!FrameFlightMode)
				vkCreateFence(CoreInterface->Device, &createInfo, 0, &fence);
			else
			{
				fences.resize(CoreInterface->MaxFramesInFlight);
				for (int i = 0; i < fences.size(); i++)
					vkCreateFence(CoreInterface->Device, &createInfo, 0, &fences[i]);
			}
		}
		Fence(Fence&) = delete;
		inline Fence(Fence&& move) noexcept {
			this->~Fence();
			this->fence = move.fence;
			this->fences = move.fences;
			move.fence = nullptr;
			move.fences.resize(0);
		}

		inline Fence& operator=(Fence&& move) noexcept {
			this->~Fence();
			this->fence = move.fence;
			this->fences = move.fences;
			move.fence = nullptr;
			move.fences.resize(0);
			return *this;
		}

		inline ~Fence() {
			if (fence)
				vkDestroyFence(_core->Device, fence, nullptr);
			for (auto fen : fences)
				vkDestroyFence(_core->Device, fen, nullptr);
		}

		inline void Wait(uint64_t timeout) {
			if (fences.size() > 0)
				::vkWaitForFences(_core->Device, 1, &fences[_core->CurrentFrame], true, timeout);
			else
				::vkWaitForFences(_core->Device, 1, &fence, true, timeout);
		}

		inline void Reset() {
			if (fences.size() > 0)
				::vkResetFences(_core->Device, 1, &fences[_core->CurrentFrame]);
			else
				::vkResetFences(_core->Device, 1, &fence);
		}

		inline VkFence& GetFence() {
			if (fences.size() > 0) {
				return fences[_core->CurrentFrame];
			}
			return fence;
		}

		inline VkFence& operator()() noexcept { return fence; }
		inline VkFence& operator()(uint32_t FrameIndex) noexcept { return fences[FrameIndex]; }
		inline VkFence& operator[](uint32_t FrameIndex) noexcept { return fences[FrameIndex]; }
		inline VkFence& operator*() noexcept { return fence; }
		VkFence fence = nullptr;
		std::vector<VkFence> fences;
	private:
		ref<VulkanCoreInterface> _core;
	};

	class Semaphore {
	public:
		Semaphore(const ref<VulkanCoreInterface>& CoreInterface, bool FrameFlightMode) : _core(CoreInterface) {
			bool TimelineSemaphore = false;
			VkSemaphoreTypeCreateInfo timelineCreateInfo{};
			timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
			timelineCreateInfo.pNext = NULL;
			timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
			timelineCreateInfo.initialValue = 0;

			VkSemaphoreCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			createInfo.pNext = TimelineSemaphore ? &timelineCreateInfo : nullptr;
			createInfo.flags = 0;
			if (FrameFlightMode) {
				semaphores.resize(CoreInterface->MaxFramesInFlight);
				for (int i = 0; i < semaphores.size(); i++)
					vkCreateSemaphore(CoreInterface->Device, &createInfo, 0, &semaphores[i]);
			}
			else
				vkCreateSemaphore(CoreInterface->Device, &createInfo, 0, &semaphore);
		}

		Semaphore(Semaphore&) = delete;
		inline Semaphore(Semaphore&& move) noexcept {
			this->~Semaphore();
			this->semaphore = move.semaphore;
			this->semaphores = move.semaphores;
			move.semaphore = nullptr;
			move.semaphores.resize(0);
		}

		inline Semaphore& operator=(Semaphore&& move) noexcept {
			this->~Semaphore();
			this->semaphore = move.semaphore;
			this->semaphores = move.semaphores;
			move.semaphore = nullptr;
			move.semaphores.resize(0);
			return *this;
		}

		inline ~Semaphore() {
			if (semaphore)
				vkDestroySemaphore(_core->Device, semaphore, nullptr);
			for (auto sema : semaphores)
				vkDestroySemaphore(_core->Device, sema, nullptr);
		}

		inline VkSemaphore& operator()() noexcept { return semaphore; }
		inline VkSemaphore& operator()(uint32_t FrameIndex) noexcept { return semaphores[FrameIndex]; }
		inline VkSemaphore& operator[](uint32_t FrameIndex) noexcept { return semaphores[FrameIndex]; }
		inline VkSemaphore& operator*() noexcept { return semaphore; }

		inline const VkSemaphore& GetSemaphore() const {
			if (semaphores.size() > 0) {
				return semaphores[_core->CurrentFrame];
			}
			return semaphore;
		}

		static inline std::vector<VkSemaphore> GetRawSemaphores(const std::vector<egx::ref<Semaphore>>& Semaphores) {
			std::vector<VkSemaphore> result(Semaphores.size());
			int i = 0;
			for (auto& s : Semaphores)
				result[i++] = s->GetSemaphore();
			return result;
		}

		VkSemaphore semaphore = nullptr;
		std::vector<VkSemaphore> semaphores;
	private:
		ref<VulkanCoreInterface> _core;
	};

	class CommandPool {

	public:
		inline CommandPool(const ref<VulkanCoreInterface>& CoreInterface, bool FrameFlightMode, bool MemoryShortLived, bool EnableIndividualReset, bool MakeProtected) :
			_core(CoreInterface), MaxFrameFlights(CoreInterface->MaxFramesInFlight) {
			VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			if (MemoryShortLived)
			{
				createInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			}
			if (EnableIndividualReset)
			{
				createInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			}
			if (MakeProtected)
			{
				createInfo.flags |= VK_COMMAND_POOL_CREATE_PROTECTED_BIT;
			}
			if (FrameFlightMode) {
				commandPools.resize(CoreInterface->MaxFramesInFlight);
				for (int i = 0; i < commandPools.size(); i++)
					vkCreateCommandPool(CoreInterface->Device, &createInfo, 0, &commandPools[i]);
			}
			else
				vkCreateCommandPool(CoreInterface->Device, &createInfo, 0, &commandPool);

		}
		CommandPool(CommandPool&) = delete;
		inline CommandPool(CommandPool&& move) noexcept {
			this->~CommandPool();
			this->commandPool = move.commandPool;
			this->commandPools = move.commandPools;
			move.commandPool = nullptr;
			move.commandPools.resize(0);
		}

		inline CommandPool& operator=(CommandPool&& move) noexcept {
			this->~CommandPool();
			this->commandPool = move.commandPool;
			this->commandPools = move.commandPools;
			move.commandPool = nullptr;
			move.commandPools.resize(0);
			return *this;
		}

		inline ~CommandPool() {
			if (commandPool)
				vkDestroyCommandPool(_core->Device, commandPool, nullptr);
			for (auto cmdPool : commandPools)
				vkDestroyCommandPool(_core->Device, cmdPool, nullptr);
		}

		inline VkCommandBuffer AllocateBuffer(bool IsPrimaryLevel) {
			assert(commandPools.size() == 0);
			VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			allocInfo.commandPool = commandPool;
			allocInfo.level = IsPrimaryLevel ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = 1;
			VkCommandBuffer buffer;
			vkAllocateCommandBuffers(_core->Device, &allocInfo, &buffer);
			return buffer;
		}

		inline std::vector<VkCommandBuffer> AllocateBufferFrameFlightMode(bool IsPrimaryLevel) {
			assert(commandPools.size() > 0);
			VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			allocInfo.level = IsPrimaryLevel ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = 1;
			std::vector<VkCommandBuffer> buffers(MaxFrameFlights);
			for (uint32_t i = 0; i < MaxFrameFlights; i++) {
				allocInfo.commandPool = commandPools[i];
				vkAllocateCommandBuffers(_core->Device, &allocInfo, &buffers[i]);
			}
			return buffers;
		}

		void Reset(VkCommandPoolResetFlags flags = 0) {
			if (commandPools.size() > 0)
				::vkResetCommandPool(_core->Device, commandPools[_core->CurrentFrame], flags);
			else
				::vkResetCommandPool(_core->Device, commandPool, flags);
		}

		inline VkCommandPool& operator()() noexcept { return commandPool; }
		inline VkCommandPool& operator()(uint32_t FrameIndex) noexcept { return commandPools[FrameIndex]; }
		inline VkCommandPool& operator[](uint32_t FrameIndex) noexcept { return commandPools[FrameIndex]; }
		inline VkCommandPool& operator*() noexcept { return commandPool; }
		VkCommandPool commandPool = nullptr;
		std::vector<VkCommandPool> commandPools;
	private:
		ref<VulkanCoreInterface> _core;
		uint32_t MaxFrameFlights;

	};

	class DescriptorPool {

	public:
		inline DescriptorPool(const ref<VulkanCoreInterface>& CoreInterface, bool FrameFlightMode, uint32_t MaximumSets, const std::vector<VkDescriptorPoolSize>& PoolSizes, VkDescriptorPoolCreateFlags flags)
			: _core(CoreInterface) {
			VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
			createInfo.flags = flags;
			createInfo.maxSets = MaximumSets;
			createInfo.poolSizeCount = (uint32_t)PoolSizes.size();
			createInfo.pPoolSizes = PoolSizes.data();
			if (FrameFlightMode) {
				pools.resize(CoreInterface->MaxFramesInFlight);
				for (int i = 0; i < pools.size(); i++)
					vkCreateDescriptorPool(CoreInterface->Device, &createInfo, 0, &pools[i]);
			}
			else
				vkCreateDescriptorPool(CoreInterface->Device, &createInfo, 0, &pool);

		}
		DescriptorPool(DescriptorPool&) = delete;
		inline DescriptorPool(DescriptorPool&& move) noexcept {
			this->~DescriptorPool();
			this->pool = move.pool;
			this->pools = move.pools;
			move.pool = nullptr;
			move.pools.resize(0);
		}

		inline DescriptorPool& operator=(DescriptorPool&& move) noexcept {
			this->~DescriptorPool();
			this->pool = move.pool;
			this->pools = move.pools;
			move.pool = nullptr;
			move.pools.resize(0);
			return *this;
		}

		inline ~DescriptorPool() {
			if (pool)
				vkDestroyDescriptorPool(_core->Device, pool, nullptr);
			for (auto pool : pools)
				vkDestroyDescriptorPool(_core->Device, pool, nullptr);
		}

		VkDescriptorPool operator()() const noexcept { return pool; }
		VkDescriptorPool operator*() const noexcept { return pool; }
		VkDescriptorPool* operator&() noexcept { return &pool; }
		VkDescriptorPool pool = nullptr;
		std::vector<VkDescriptorPool> pools;
	private:
		ref<VulkanCoreInterface> _core;

	};


	inline static ref<Fence> CreateFence(const ref<VulkanCoreInterface>& CoreInterface, bool CreateSignaled, bool FrameFlightMode) { return { new Fence(CoreInterface, CreateSignaled, FrameFlightMode) }; }
	inline static ref<Semaphore> CreateSemaphore(const ref<VulkanCoreInterface>& CoreInterface, bool FrameFlightMode) { return { new Semaphore(CoreInterface, FrameFlightMode) }; }
	inline static ref<CommandPool> CreateCommandPool(
		const ref<VulkanCoreInterface>& CoreInterface,
		bool FrameFlightMode,
		bool MemoryShortLived, bool EnableIndividualReset, bool MakeProtected) {
		return { new CommandPool(CoreInterface, FrameFlightMode, MemoryShortLived, EnableIndividualReset, MakeProtected) };
	}

	inline static void StartCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags usage) {
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = usage;
		vkBeginCommandBuffer(buffer, &beginInfo);
	}

	inline static ref<DescriptorPool> CreateDescriptorPool(ref<VulkanCoreInterface>& CoreInterface, bool FrameFlightMode, uint32_t MaximumSets, const std::vector<VkDescriptorPoolSize>& PoolSizes, VkDescriptorPoolCreateFlags flags)
	{
		return { new DescriptorPool(CoreInterface, FrameFlightMode, MaximumSets, PoolSizes, flags) };
	}

	inline static void SubmitCmdBuffers(VkQueue Queue,
		const std::vector<VkCommandBuffer>& CmdBuffers,
		const std::vector<VkSemaphore>& WaitSemaphores,
		const std::vector<VkPipelineStageFlags>& WaitDstStageMask,
		const std::vector<VkSemaphore>& SignalSemaphores,
		VkFence SignalFence,
		bool Block = false) {
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

	inline static void SubmitCmdBuffers(ref<VulkanCoreInterface>& CoreInterface,
		const std::vector<VkCommandBuffer>& CmdBuffers,
		const std::vector<VkSemaphore>& WaitSemaphores,
		const std::vector<VkPipelineStageFlags>& WaitDstStageMask,
		const std::vector<VkSemaphore>& SignalSemaphores,
		VkFence SignalFence,
		bool Block = false) {
		SubmitCmdBuffers(CoreInterface->Queue, CmdBuffers, WaitSemaphores, WaitDstStageMask, SignalSemaphores, SignalFence, Block);
	}

	inline static VkQueryPool CreateQueryPool(ref<VulkanCoreInterface>& CoreInterface, VkQueryType QueryType, uint32_t QueryCount, VkQueryPipelineStatisticFlags PipelineStatistics)
	{
		VkQueryPool query;
		VkQueryPoolCreateInfo createInfo{};
		vkCreateQueryPool(CoreInterface->Device, &createInfo, nullptr, &query);
		return query;
	}

	class SingleUseBuffer {
	public:
		SingleUseBuffer(const ref<VulkanCoreInterface>& CoreInterface) : Core(CoreInterface) {
			Pool = CreateCommandPool(CoreInterface, false, true, false, false);
			Cmd = Pool->AllocateBuffer(true);
			Fence = CreateFence(CoreInterface, false, false);
			StartCommandBuffer(Cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		}

		SingleUseBuffer(SingleUseBuffer& cp) = delete;
		SingleUseBuffer(SingleUseBuffer&& mv) = delete;
		SingleUseBuffer& operator=(SingleUseBuffer&& mv) = delete;

		void Execute() const noexcept {
			vkEndCommandBuffer(Cmd);
			SubmitCmdBuffers(Core->Queue, { Cmd }, {}, {}, {}, **Fence);
			::vkWaitForFences(Core->Device, 1, &Fence->fence, true, UINT64_MAX);
		}

		void Reset() const noexcept {
			::vkResetCommandPool(Core->Device, **Pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
			::vkResetFences(Core->Device, 1, &Fence->fence);
		}

		ref<CommandPool> Pool;
		VkCommandBuffer Cmd;
		ref<Fence> Fence;
		ref<VulkanCoreInterface> Core;
	};

	void EGX_API InsertDebugLabel(ref<VulkanCoreInterface>& CoreInterface, VkCommandBuffer cmd, uint32_t FrameIndex, const std::string_view DebugLabelText, float r, float g, float b);
	void EGX_API EndDebugLabel(ref<VulkanCoreInterface>& CoreInterface, VkCommandBuffer cmd);

	struct FrameData {
		ref<Fence> Fence;
		ref<Semaphore> Semaphore;
		ref<CommandPool> Pool;
		std::vector<VkCommandBuffer> Cmds;
	};

	std::string VkResultToString(VkResult result);

	EGX_API ImFont* LoadFont(const ref<VulkanCoreInterface>& CoreInterface, std::string_view path, float size);

}
