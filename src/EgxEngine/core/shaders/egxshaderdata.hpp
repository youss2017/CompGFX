#pragma once
#include "../egxcommon.hpp"
#include "../memory/egxmemory.hpp"
#include <vector>
#include <map>

namespace egx {

	class egxsetpool {
	public:
		static ref<egxsetpool> EGX_API  FactoryCreate(ref<VulkanCoreInterface>& CoreInterface);
		EGX_API egxsetpool(egxsetpool& copy) = delete;
		EGX_API egxsetpool(egxsetpool&& move) noexcept;
		EGX_API egxsetpool& operator=(egxsetpool& move) noexcept;
		EGX_API ~egxsetpool();

		void EGX_API IncrementSetCount();
		void EGX_API AddType(VkDescriptorType type, uint32_t count);

		VkDescriptorPool EGX_API getpool();

	protected:
		egxsetpool(ref<VulkanCoreInterface>& CoreInterface) :
			_coreinterface(CoreInterface) {}

	protected:
		VkDescriptorPool _pool = nullptr;
		ref<VulkanCoreInterface>& _coreinterface;
		std::map<VkDescriptorType, uint32_t> _types;
		uint32_t _setcount = 0;
	};

	namespace _internal {
		struct bufferinformation {
			uint32_t BindingId{};
			VkShaderStageFlags Stages{};
			VkDescriptorType Type{};
			memorylayout Layout{};
			size_t Size{};
			size_t BlockSize{};
			ref<Buffer> Buffer;
		};

		struct imageinformation {
			uint32_t BindingId{};
			VkShaderStageFlags Stages{};
			VkDescriptorType Type{};
			VkSampler Sampler{};
			std::vector<VkImageLayout> ImageLayout;
			std::vector<ref<Image>> Image;
		};

	}

	class egxshaderset {
	public:
		static egx::ref<egx::egxshaderset> EGX_API
			FactoryCreate(
				ref<VulkanCoreInterface>& CoreInterface,
				uint32_t SetId,
				ref<egxsetpool> Pool
			);

		static egx::ref<egx::egxshaderset> EGX_API
			FactoryCreate(
				ref<VulkanCoreInterface>& CoreInterface,
				uint32_t SetId
			);
		EGX_API egxshaderset(egxshaderset&& move) noexcept;
		EGX_API egxshaderset& operator=(egxshaderset& move) noexcept;
		EGX_API ~egxshaderset() noexcept;

		/// <summary>
		/// Set BlockSize to 0 for the entire range of buffer
		/// If you are using dynamic offsets then BlockSize is the size of the struct, ex:
		/// struct data { vec4 pos; vec4 color; }
		/// Therefore BlockSize is 16 bytes
		/// If you want 4 blocks then Size should be BlockSize * 4
		/// </summary>
		EGX_API void CreateBuffer(
			uint32_t BindingId,
			VkShaderStageFlags Stages,
			VkDescriptorType Type,
			memorylayout Layout,
			uint32_t Size,
			uint32_t BlockSize = 0);

		/// <summary>
		/// Set BlockSize to 0 for the entire range of buffer
		/// If you are using dynamic offsets then BlockSize is the size of the struct, ex:
		/// struct data { vec4 pos; vec4 color; }
		/// Therefore BlockSize is 16 bytes
		/// If you want 4 blocks then Size should be BlockSize * 4
		/// </summary>
		void EGX_API AddBuffer(
			uint32_t BindingId,
			VkShaderStageFlags Stages,
			VkDescriptorType Type,
			ref<Buffer> Buffer,
			uint32_t BlockSize = 0);

		// View index 0 is used
		EGX_API void AddImage(
			uint32_t BindingId,
			VkShaderStageFlags Stages,
			VkDescriptorType Type,
			VkSampler Sampler,
			ref<Image> Image,
			VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		// View index 0 is used
		EGX_API void AddImageArray(
			uint32_t BindingId,
			VkShaderStageFlags Stages,
			VkDescriptorType Type,
			VkSampler Sampler,
			const std::vector<ref<Image>>& Images,
			VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);


		// View index 0 is used
		EGX_API void AddImageArray(
			uint32_t BindingId,
			VkShaderStageFlags Stages,
			VkDescriptorType Type,
			VkSampler Sampler,
			const std::vector<ref<Image>>& Images,
			std::vector<VkImageLayout> ImageLayouts
		);

		EGX_API VkDescriptorSet& GetSet();
		EGX_API VkDescriptorSetLayout& GetSetLayout();

		inline void SetDynamicOffset(uint32_t BindingId, uint32_t Offset) noexcept {
			DynamicOffsets[BindingId] = Offset;
		}

	protected:
		egxshaderset(ref<VulkanCoreInterface>& CoreInterface, uint32_t SetId, ref<egxsetpool> pool) :
			_coreinterface(CoreInterface), SetId(SetId), _pool(pool)
		{
			_pool->IncrementSetCount();
		}
		EGX_API egxshaderset(egxshaderset& copy) = delete;

	public:
		const uint32_t SetId;
		std::map<uint32_t, egx::ref<Buffer>> Buffers;
		std::map<uint32_t, uint32_t> DynamicOffsets;

	protected:
		ref<VulkanCoreInterface> _coreinterface;
		std::map<uint32_t, _internal::bufferinformation> _buffers;
		std::map<uint32_t, _internal::imageinformation> _images;
		ref<egxsetpool> _pool;
		VkDescriptorSet _set = nullptr;
		VkDescriptorSetLayout _setlayout = nullptr;
	};

	class PipelineLayout {

	public:
		static ref<PipelineLayout> EGX_API FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface);
		EGX_API ~PipelineLayout();
		EGX_API PipelineLayout(PipelineLayout&) = delete;
		EGX_API PipelineLayout(PipelineLayout&&) noexcept;
		EGX_API PipelineLayout& operator=(PipelineLayout&) noexcept;

		EGX_API void AddSet(ref<egxshaderset>& set);
		EGX_API void AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages);
		EGX_API VkPipelineLayout& GetLayout();

		inline void SetDynamicOffset(uint32_t SetId, uint32_t BindingId, uint32_t Offset) {
#ifdef _DEBUG
			assert(Sets[SetId]->Buffers[BindingId].IsValidRef());
			auto type = Sets[SetId]->Buffers[BindingId]->Type;
			assert(type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
				type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC && "Must be a dynamic buffer type");
			if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
				assert(egx::MinAlignmentForStorage(_coreinterface, Offset) == Offset && "Offset must align to min alignment for storage");
			}
			else {
				assert(egx::MinAlignmentForUniform(_coreinterface, Offset) == Offset && "Offset must align to min alignment for uniform");
			}
#endif
			Sets[SetId]->DynamicOffsets[BindingId] = Offset;
		}
		inline void bind(VkCommandBuffer cmd, VkPipelineBindPoint BindPoint) {
			if (_cached_set.size() == 0) return;
			UpdateDynamicOffsets();
			vkCmdBindDescriptorSets(cmd, BindPoint, GetLayout(), 0, (uint32_t)_cached_set.size(), _cached_set.data(), (uint32_t)_cached_dynamicoffsets.size(), _cached_dynamicoffsets.data());
		}

	protected:
		EGX_API PipelineLayout(const ref<VulkanCoreInterface>& CoreInterface) :
			_coreinterface(CoreInterface)
		{}

		inline void UpdateDynamicOffsets() {
			if (_cached_dynamicoffsets.size() == 0) return;
			int j = 0;
			for (auto& [id, set] : Sets) {
				for (auto& [bId, offset] : set->DynamicOffsets)
					_cached_dynamicoffsets[j] = offset;
			}
		}

	public:
		std::map<uint32_t, ref<egxshaderset>> Sets;

	protected:
		ref<VulkanCoreInterface> _coreinterface;
		std::vector<VkPushConstantRange> _ranges;
		std::vector<VkDescriptorSet> _cached_set;
		std::vector<uint32_t> _cached_dynamicoffsets;
		VkPipelineLayout _layout = nullptr;
	};

}
