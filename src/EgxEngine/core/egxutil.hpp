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
				commandPool = commandPools[0];
			}
			else
				vkCreateCommandPool(CoreInterface->Device, &createInfo, 0, &commandPool);

		}
		CommandPool(CommandPool&) = delete;
		inline CommandPool(CommandPool&& move) noexcept {
			this->~CommandPool();
			memcpy(this, &move, sizeof(CommandPool));
			memset(&move, 0, sizeof(CommandPool));
		}

		inline CommandPool& operator=(CommandPool&& move) noexcept {
			this->~CommandPool();
			memcpy(this, &move, sizeof(CommandPool));
			memset(&move, 0, sizeof(CommandPool));
			return *this;
		}

		inline ~CommandPool() {
			if (commandPool)
				vkDestroyCommandPool(_core->Device, commandPool, nullptr);
			for (auto cmdPool : commandPools)
				vkDestroyCommandPool(_core->Device, cmdPool, nullptr);
		}

		inline VkCommandBuffer AllocateBuffer(bool IsPrimaryLevel) {
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

		void FreeCommandBuffer(VkCommandBuffer cmd) {
			vkFreeCommandBuffers(_core->Device, commandPool, 1, &cmd);
		}

		void FreeCommandBuffers(const std::vector<VkCommandBuffer>& cmds) {
			for(size_t i = 0; i < cmds.size(); i++)
				vkFreeCommandBuffers(_core->Device, commandPools[i], 1u, &cmds[i]);
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


	inline static ref<CommandPool> CreateCommandPool(
		const ref<VulkanCoreInterface>& CoreInterface,
		bool FrameFlightMode, bool MemoryShortLived, bool EnableIndividualReset, bool MakeProtected) {
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

	inline static VkQueryPool CreateQueryPool(ref<VulkanCoreInterface>& CoreInterface, VkQueryType QueryType, uint32_t QueryCount, VkQueryPipelineStatisticFlags PipelineStatistics)
	{
		VkQueryPool query;
		VkQueryPoolCreateInfo createInfo{};
		vkCreateQueryPool(CoreInterface->Device, &createInfo, nullptr, &query);
		return query;
	}

	void EGX_API InsertDebugLabel(ref<VulkanCoreInterface>& CoreInterface, VkCommandBuffer cmd, uint32_t FrameIndex, const std::string_view DebugLabelText, float r, float g, float b);
	void EGX_API EndDebugLabel(ref<VulkanCoreInterface>& CoreInterface, VkCommandBuffer cmd);

	//struct FrameData {
	//	ref<Fence> Fence;
	//	ref<Semaphore> Semaphore;
	//	ref<CommandPool> Pool;
	//	std::vector<VkCommandBuffer> Cmds;
	//};

	std::string VkResultToString(VkResult result);

	EGX_API ImFont* LoadFont(const ref<VulkanCoreInterface>& CoreInterface, std::string_view path, float size);

}
