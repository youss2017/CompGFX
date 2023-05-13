#pragma once
#include "../egxcommon.hpp"
#include "../memory/frameflight.hpp"
#include "../memory/egxmemory.hpp"
#include "../pipeline/egxsampler.hpp"
#include <map>
#include <vector>

namespace egx
{
	class DescriptorSet;

	struct SetPoolRequirementsInfo
	{
		std::map<VkDescriptorType, uint32_t> Type;

		static SetPoolRequirementsInfo Inline(VkDescriptorType Type, uint32_t Count) {
			SetPoolRequirementsInfo req;
			req.Type[Type] = Count;
			return req;
		}

		void ExpandCurrentReqInfo(const SetPoolRequirementsInfo& MoreRequirements)
		{
			for (auto& [type, count] : MoreRequirements.Type)
			{
				Type[type] += count;
			}
		}

		std::vector<VkDescriptorPoolSize> GeneratePoolSizes(uint32_t MaxFrames) const
		{
			std::vector<VkDescriptorPoolSize> ret;
			for (auto& [type, count] : Type)
			{
				auto& size = ret.emplace_back();
				size.type = type;
				size.descriptorCount = count * MaxFrames;
			}
			return ret;
		}
	};

	class SetPool : public FrameFlight
	{
	public:
		EGX_API static ref<SetPool> FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface);
		EGX_API SetPool(SetPool& copy) = delete;
		EGX_API SetPool(SetPool&& move) noexcept;
		EGX_API SetPool& operator=(SetPool& move) noexcept;
		EGX_API ~SetPool();

		EGX_API void AdjustForRequirements(const SetPoolRequirementsInfo& Requirements);
		EGX_API void AdjustForRequirements(const std::initializer_list<const egx::ref<DescriptorSet>>& Sets);
		void AdjustForRequirements(const egx::ref<DescriptorSet>& Set)
		{
			AdjustForRequirements({ Set });
		}

		void AdjustForRequirements(const std::map<uint32_t, ref<DescriptorSet>>& SetIdMap) {
			for (auto& [id, set] : SetIdMap) {
				AdjustForRequirements(set);
			}
		}

		void IncrementSetCount(uint32_t Count = 1) { _SetCount += Count; }

		EGX_API VkDescriptorPool GetSetPool();

	protected:
		SetPool(const ref<VulkanCoreInterface>& CoreInterface) : _coreinterface(CoreInterface)
		{
			DelayInitalizeFF(CoreInterface);
		}

	protected:
		VkDescriptorPool _pool = nullptr;
		ref<VulkanCoreInterface> _coreinterface;
		SetPoolRequirementsInfo _pool_requirements;
		uint32_t _SetCount = 0;
	};

	namespace internal
	{
		struct BufferDescription
		{
			uint32_t BindingId = 0;
			VkDescriptorType Type{};
			VkShaderStageFlags ShaderStages{};
			size_t Offset = 0;
			size_t StructSize = 0;
			std::vector<bool> IssueUpdateList;
			ref<Buffer> BufferRef = { nullptr };
		};
		struct ImageDescription
		{
			uint32_t BindingId{};
			uint32_t DescriptorCount{};
			VkDescriptorType Type{};
			VkShaderStageFlags ShaderStages{};
			std::vector<bool> IssueUpdateList;
			std::vector<ref<Image>> ImageRef;
			std::vector<ref<Sampler>> ImageSamplers;
			std::vector<VkImageLayout> ImageLayouts;
			std::vector<uint32_t> ViewIndex = { 0 };
			bool IsInputAttachment{ false };
		};
	}

	class DescriptorSet : public FrameFlight
	{
	public:
		EGX_API static ref<DescriptorSet> FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface, uint32_t SetId);

		EGX_API DescriptorSet(DescriptorSet&) = delete;
		EGX_API DescriptorSet(DescriptorSet&&) noexcept;
		EGX_API ~DescriptorSet() noexcept;
		EGX_API DescriptorSet& operator=(DescriptorSet&) noexcept;

		EGX_API void DescribeBuffer(uint32_t BindingId, VkDescriptorType Type, VkShaderStageFlags ShaderStages);
		// Must be called before bound to Cmd Buffer (Auto Sync for frame in flight)
		EGX_API void SetBuffer(uint32_t BindingId, const ref<Buffer>& BufferRef, uint32_t StructSize, uint32_t Offset);

		EGX_API void DescribeImage(uint32_t BindingId, uint32_t DescriptorCount, VkDescriptorType Type, VkShaderStageFlags ShaderStages);
		// Must be called before bound to Cmd Buffer (Auto Sync for frame in flight).
		// If only one Sampler is in Samplers then that Sampler will be used for all images, same for Image Layouts.
		// If images vector is more than 1 then binding is assumed to be an array.
		// Same for View Index
		EGX_API void SetImage(
			uint32_t BindingId,
			const std::vector<ref<Image>>& Images,
			const std::vector<ref<Sampler>>& Samplers,
			const std::vector<VkImageLayout>& ImageLayouts,
			const std::vector<uint32_t>& ViewIndex,
			bool inputAttachment = false);

		EGX_API SetPoolRequirementsInfo GetDescriptorPoolRequirements() const;

		// You must call DescribeBuffer()/DescribeImage() before this function
		// Once this function is called, the descriptor set will be created.
		EGX_API const VkDescriptorSetLayout& GetSetLayout();

		EGX_API void AllocateSet(const ref<SetPool>& Pool);

		// You must call DescribeBuffer()/DescribeImage() before this function
		// Once this function is called, the descriptor set will be created.
		EGX_API const VkDescriptorSet& GetSet();

	private:
		EGX_API DescriptorSet(const ref<VulkanCoreInterface>& CoreInterface, uint32_t SetId);

	public:
		const uint32_t SetId;

	private:
		friend class PipelineLayout;
		ref<VulkanCoreInterface> _core_interface;
		std::map<uint32_t, internal::BufferDescription> _buffer_descriptions;
		std::map<uint32_t, internal::ImageDescription> _image_descriptions;
		VkDescriptorSetLayout _set_layout = nullptr;
		std::vector<VkWriteDescriptorSet> _set_writes;
		std::vector<VkDescriptorBufferInfo> _set_write_buffer_info;
		std::vector<VkDescriptorImageInfo> _set_write_image_info;
		std::vector<VkDescriptorSet> _set;
		std::map<uint32_t, uint32_t> _set_binding_to_dynamic_offset;
		uint32_t _perform_writes = 0;
	};

	class PipelineLayout
	{
	public:
		static ref<PipelineLayout> EGX_API FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface);
		EGX_API ~PipelineLayout();
		EGX_API PipelineLayout(PipelineLayout&) = delete;
		EGX_API PipelineLayout(PipelineLayout&&) noexcept;
		EGX_API PipelineLayout& operator=(PipelineLayout&) noexcept;

		EGX_API void AddSets(const std::initializer_list<const ref<DescriptorSet>>& set);
		EGX_API void AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages);
		EGX_API const VkPipelineLayout& GetLayout();

		inline void SetDynamicOffset(uint32_t SetId, uint32_t BindingId, uint32_t Offset)
		{
#ifdef _DEBUG
			auto& bufdesc = Sets[SetId]->_buffer_descriptions[BindingId];
			assert(bufdesc.BufferRef.IsValidRef());
			auto type = bufdesc.Type;
			assert(type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
				type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC && "Must be a dynamic buffer type");
			if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
			{
				assert(egx::MinAlignmentForStorage(_coreinterface, Offset) == Offset && "Offset must align to min alignment for storage");
			}
			else
			{
				assert(egx::MinAlignmentForUniform(_coreinterface, Offset) == Offset && "Offset must align to min alignment for uniform");
			}
			assert(Sets[SetId]->_set_binding_to_dynamic_offset.find(BindingId) != Sets[SetId]->_set_binding_to_dynamic_offset.end() &&
				"BindingId does not exist or is not dynamic.");
#endif
			Sets[SetId]->_set_binding_to_dynamic_offset[BindingId] = Offset;
		}

		inline void Bind(VkCommandBuffer cmd, VkPipelineBindPoint BindPoint)
		{
			if (Sets.size() > 0) {
				StructureDataForBinding();
				vkCmdBindDescriptorSets(cmd, BindPoint, GetLayout(), 0, (uint32_t)_sets.size(), _sets.data(), (uint32_t)_dynamic_offsets.size(), _dynamic_offsets.data());
			}
		}

	protected:
		PipelineLayout(const ref<VulkanCoreInterface>& CoreInterface) : _coreinterface(CoreInterface)
		{}

		inline void StructureDataForBinding()
		{
			_dynamic_offsets.clear();
			_sets.clear();
			for (auto& [id, set] : Sets) {
				for (auto& [bid, offset] : set->_set_binding_to_dynamic_offset) {
					_dynamic_offsets.push_back(offset);
				}
				_sets.push_back(set->GetSet());
			}
		}

	public:
		std::map<uint32_t, ref<DescriptorSet>> Sets;

	protected:
		ref<VulkanCoreInterface> _coreinterface;
		std::vector<VkPushConstantRange> _ranges;
		std::vector<VkDescriptorSet> _sets;
		std::vector<uint32_t> _dynamic_offsets;
		VkPipelineLayout _layout = nullptr;
	};

}