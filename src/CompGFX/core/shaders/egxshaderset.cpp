#include "egxshaderset.hpp"
#include "../memory/formatsize.hpp"
#include <Utility/CppUtility.hpp>

using namespace egx;

egx::ref<egx::SetPool> egx::SetPool::FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface)
{
	return ref<SetPool>(new SetPool(CoreInterface));
}

egx::SetPool::SetPool(SetPool&& move) noexcept
	: _coreinterface(move._coreinterface)
{
	this->~SetPool();
	memcpy(&move, this, sizeof(SetPool));
	memset(&move, 0, sizeof(SetPool));
}

egx::SetPool& egx::SetPool::operator=(SetPool& move) noexcept
{
	this->~SetPool();
	memcpy(&move, this, sizeof(SetPool));
	memset(&move, 0, sizeof(SetPool));
	return *this;
}

egx::SetPool::~SetPool()
{
	if (_pool)
		vkDestroyDescriptorPool(_coreinterface->Device, _pool, nullptr);
}

void SetPool::AdjustForRequirements(const SetPoolRequirementsInfo& Requirements)
{
	assert(_pool == nullptr);
	_pool_requirements.ExpandCurrentReqInfo(Requirements);
}

void SetPool::AdjustForRequirements(const std::initializer_list<const egx::ref<DescriptorSet>>& Sets)
{
	for (const auto& set : Sets)
		AdjustForRequirements(set->GetDescriptorPoolRequirements());
	IncrementSetCount((uint32_t)Sets.size());
}

VkDescriptorPool egx::SetPool::GetSetPool()
{
	if (_pool != nullptr)
		return _pool;
	VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

	std::vector<VkDescriptorPoolSize> sizes = _pool_requirements.GeneratePoolSizes(_coreinterface->MaxFramesInFlight);

	createInfo.flags = 0;
	createInfo.maxSets = _SetCount * _max_frames;
	createInfo.poolSizeCount = (uint32_t)sizes.size();
	createInfo.pPoolSizes = sizes.data();
	vkCreateDescriptorPool(_coreinterface->Device, &createInfo, nullptr, &_pool);
	return _pool;
}

ref<DescriptorSet> DescriptorSet::FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface, uint32_t DebugSetId)
{
	return new DescriptorSet(CoreInterface, DebugSetId);
}

DescriptorSet::DescriptorSet(DescriptorSet&& move) noexcept : SetId(move.SetId)
{
	memcpy(this, &move, sizeof(DescriptorSet));
	memset(&move, 0, sizeof(DescriptorSet));
}
DescriptorSet::~DescriptorSet() noexcept
{
	if (_set_layout)
	{
		vkDestroyDescriptorSetLayout(_core_interface->Device, _set_layout, nullptr);
	}
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet& move) noexcept
{
	memcpy(this, &move, sizeof(DescriptorSet));
	memset(&move, 0, sizeof(DescriptorSet));
	return *this;
}

void DescriptorSet::DescribeBuffer(uint32_t BindingId, VkDescriptorType Type, VkShaderStageFlags ShaderStages)
{
	assert(_set_layout == nullptr);
	auto& desc = _buffer_descriptions[BindingId];
	desc.BindingId = BindingId;
	desc.Type = Type;
	desc.ShaderStages = ShaderStages;
	desc.IssueUpdateList.resize(_max_frames, false);
	_set_writes.emplace_back();
	if (Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
		_set_binding_to_dynamic_offset[BindingId] = 0;
}

// Must be called before bound to Cmd Buffer (Auto Sync for frame in flight)
void DescriptorSet::SetBuffer(uint32_t BindingId, const ref<Buffer>& BufferRef, uint32_t StructSize, uint32_t Offset)
{
#ifdef _DEBUG
	if (!_buffer_descriptions.contains(BindingId)) {
		LOGEXCEPT("SetBuffer(Binding={0}) was not described to descriptor set.", BindingId);
	}
#endif
	auto& desc = _buffer_descriptions.at(BindingId);
	desc.BindingId = BindingId;
	desc.StructSize = StructSize == 0 ? BufferRef->Size : StructSize;
	desc.Offset = Offset;
	desc.BufferRef = BufferRef;
	std::fill(desc.IssueUpdateList.begin(), desc.IssueUpdateList.end(), true);
	_set_write_buffer_info.reserve(1);
	_perform_writes = _max_frames;
}

void DescriptorSet::DescribeImage(uint32_t BindingId, uint32_t DescriptorCount, VkDescriptorType Type, VkShaderStageFlags ShaderStages)
{
	assert(_set_layout == nullptr);
	auto& desc = _image_descriptions[BindingId];
	desc.BindingId = BindingId;
	desc.Type = Type;
	desc.ShaderStages = ShaderStages;
	desc.DescriptorCount = DescriptorCount;
	desc.IssueUpdateList.resize(_max_frames, false);
	_set_writes.emplace_back();
}
// Must be called before bound to Cmd Buffer (Auto Sync for frame in flight).
// If only one Sampler is in Samplers then that Sampler will be used for all images, same for Image Layouts.
// If images vector is more than 1 then binding is assumed to be an array.
void DescriptorSet::SetImage(
	uint32_t BindingId,
	const std::vector<ref<Image>>& Images,
	const std::vector<ref<Sampler>>& Samplers,
	const std::vector<VkImageLayout>& ImageLayouts,
	const std::vector<uint32_t>& ViewIndex,
	bool inputAttachment)
{

#ifdef _DEBUG
	if (!_image_descriptions.contains(BindingId)) {
		LOGEXCEPT("SetImage(Binding={0}) was not described to descriptor set.", BindingId);
	}
#endif
	auto& desc = _image_descriptions.at(BindingId);
	desc.BindingId = BindingId;
	desc.ImageRef = Images;
	desc.ImageSamplers = Samplers;
	desc.ImageLayouts = ImageLayouts;
	desc.ViewIndex = ViewIndex;
	desc.IsInputAttachment = inputAttachment;
	std::fill(desc.IssueUpdateList.begin(), desc.IssueUpdateList.end(), true);
	_set_write_image_info.reserve(Images.size());
	_perform_writes = _max_frames;
}

SetPoolRequirementsInfo DescriptorSet::GetDescriptorPoolRequirements() const
{
	SetPoolRequirementsInfo req;
	for (auto& [id, buffer] : _buffer_descriptions)
	{
		req.Type[buffer.Type] += 1;
	}
	for (auto& [id, image] : _image_descriptions)
	{
		req.Type[image.Type] += image.DescriptorCount;
	}
	return req;
}

// You must call DescribeBuffer()/DescribeImage() before this function
// Once this function is called, the descriptor set will be created.
const VkDescriptorSetLayout& DescriptorSet::GetSetLayout()
{
	if (_set_layout)
		return _set_layout;
	VkDescriptorSetLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for (auto& [Id, info] : _buffer_descriptions)
	{
		auto& binding = bindings.emplace_back();
		binding.binding = info.BindingId;
		binding.descriptorType = info.Type;
		binding.descriptorCount = 1;
		binding.stageFlags = info.ShaderStages;
		binding.pImmutableSamplers = nullptr;
	}
	for (auto& [Id, info] : _image_descriptions)
	{
		auto& binding = bindings.emplace_back();
		binding.binding = info.BindingId;
		binding.descriptorType = info.Type;
		binding.descriptorCount = info.DescriptorCount;
		binding.stageFlags = info.ShaderStages;
		binding.pImmutableSamplers = nullptr;
	}
	createInfo.bindingCount = (uint32_t)bindings.size();
	createInfo.pBindings = bindings.data();
	vkCreateDescriptorSetLayout(_core_interface->Device, &createInfo, nullptr, &_set_layout);
	return _set_layout;
}

void DescriptorSet::AllocateSet(const ref<SetPool>& Pool)
{
	_set.resize(_max_frames);
	VkDescriptorSetLayout setlayout = GetSetLayout();
	VkDescriptorPool pool = Pool->GetSetPool();
	VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = pool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &setlayout;
	for (size_t i = 0; i < _set.size(); i++) {
		VkResult ret = vkAllocateDescriptorSets(_core_interface->Device, &allocateInfo, &_set[i]);
		if (ret != VK_SUCCESS) {
			LOG(ERR, "Could not allocate descriptor set; (Out Of Memory? Did you allocate enought Types/Sets) {0}", ret);
		}
	}
}

// You must call DescribeBuffer()/DescribeImage() before this function
// Once this function is called, the descriptor set will be created.
const VkDescriptorSet& DescriptorSet::GetSet()
{
	assert(_set.size() > 0);
	uint32_t frame = GetCurrentFrame();
	if (_perform_writes == 0)
		return _set[frame];
	/*
		Note: Pointer will remain valid even after emplace_back()
		because std::vector::reserve() is called in SetBuffer() and SetImage()
	*/
	_set_writes.clear();
	_set_write_buffer_info.clear();
	_set_write_image_info.clear();

	// We have to reserve so the vector does not reallocate memory
	// if the vector reallocates memory then memory pointers will be invalid.
	_set_write_buffer_info.reserve(_buffer_descriptions.size());

	VkDescriptorSet set = _set[frame];
	for (auto& [id, buffer] : _buffer_descriptions)
	{
		if (buffer.IssueUpdateList.size() > 0 && buffer.IssueUpdateList[frame])
		{
			auto& bufferInfo = _set_write_buffer_info.emplace_back();
			bufferInfo.buffer = buffer.BufferRef->GetBuffer();
			bufferInfo.offset = buffer.Offset;
			bufferInfo.range = buffer.StructSize;
			auto& write = _set_writes.emplace_back();
			write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.dstSet = set;
			write.dstBinding = id;
			write.dstArrayElement = 0;
			write.descriptorCount = 1;
			write.descriptorType = buffer.Type;
			write.pBufferInfo = &bufferInfo;
			buffer.IssueUpdateList[frame] = false;
		}
	}
	for (auto& [id, image] : _image_descriptions)
	{
		_set_write_image_info.resize(_set_write_image_info.size() + image.ImageRef.size());
	}
	for (auto& [id, image] : _image_descriptions)
	{
		if (image.IssueUpdateList.size() > 0 && image.IssueUpdateList[frame])
		{
			size_t counter = _set_write_image_info.size();
			VkDescriptorImageInfo* pImageInfos = new VkDescriptorImageInfo[image.ImageRef.size()]{};
			for (size_t i = 0; i < image.ImageRef.size(); i++)
			{
				uint32_t viewIndex = image.ViewIndex.size() == 1 ? image.ViewIndex[0] : image.ViewIndex[i];
				auto& imageInfo = pImageInfos[i];
				if (image.ImageSamplers.size() > 0) imageInfo.sampler = image.ImageSamplers.size() > 1 ? image.ImageSamplers[i]->GetSampler() : image.ImageSamplers[0]->GetSampler();
				else imageInfo.sampler = nullptr;
				imageInfo.imageView = image.ImageRef[i]->view(viewIndex);
				imageInfo.imageLayout = image.ImageLayouts.size() > 1 ? image.ImageLayouts[i] : image.ImageLayouts[0];
			}
			auto& write = _set_writes.emplace_back();
			write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.dstSet = set;
			write.dstBinding = id;
			write.dstArrayElement = 0;
			write.descriptorCount = (uint32_t)image.ImageRef.size();
			write.descriptorType = image.Type;
			write.pImageInfo = pImageInfos;
			image.IssueUpdateList[frame] = false;
		}
	}
	vkUpdateDescriptorSets(_core_interface->Device, (uint32_t)_set_writes.size(), _set_writes.data(), 0, nullptr);
	_perform_writes--;

	for (auto& item : _set_writes) {
		if (item.pImageInfo) delete[] item.pImageInfo;
	}

	return _set[frame];
}

DescriptorSet::DescriptorSet(const ref<VulkanCoreInterface>& CoreInterface, uint32_t SetId)
	: _core_interface(CoreInterface), SetId(SetId)
{
	DelayInitalizeFF(CoreInterface);
}

#pragma region pipeline layout
egx::ref<egx::PipelineLayout> egx::PipelineLayout::FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface)
{
	return ref<PipelineLayout>(new PipelineLayout(CoreInterface));
}

egx::PipelineLayout::~PipelineLayout()
{
	if (_layout)
		vkDestroyPipelineLayout(_coreinterface->Device, _layout, nullptr);
}

egx::PipelineLayout::PipelineLayout(PipelineLayout&& move) noexcept
{
	this->~PipelineLayout();
	memcpy(this, &move, sizeof(PipelineLayout));
	memset(&move, 0, sizeof(PipelineLayout));
}

egx::PipelineLayout& egx::PipelineLayout::operator=(PipelineLayout& move) noexcept
{
	this->~PipelineLayout();
	memcpy(this, &move, sizeof(PipelineLayout));
	memset(&move, 0, sizeof(PipelineLayout));
	return *this;
}

void egx::PipelineLayout::AddSets(const std::initializer_list<const ref<DescriptorSet>>& sets)
{
	assert(_layout == nullptr);
	for (auto& set : sets)
		Sets[set->SetId] = set;
}

void egx::PipelineLayout::AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages)
{
	assert(_layout == nullptr);
	_ranges.push_back({ stages, offset, size });
}

const VkPipelineLayout& egx::PipelineLayout::GetLayout()
{
	if (_layout) return _layout;
	std::vector<VkDescriptorSetLayout> setLayouts(Sets.size());
	for (int i = 0; i < setLayouts.size(); i++) {
		setLayouts[i] = Sets[i]->GetSetLayout();
	}
	VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	createInfo.setLayoutCount = (uint32_t)setLayouts.size();
	createInfo.pSetLayouts = setLayouts.data();
	createInfo.pushConstantRangeCount = (uint32_t)_ranges.size();
	createInfo.pPushConstantRanges = _ranges.data();
	vkCreatePipelineLayout(_coreinterface->Device, &createInfo, nullptr, &_layout);
	return _layout;
}

#pragma endregion