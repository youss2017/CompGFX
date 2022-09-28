#include "egxshaderdata.hpp"

#pragma region shader set
egx::ref<egx::egxshaderset> egx::egxshaderset::FactoryCreate(ref<VulkanCoreInterface>& CoreInterface, uint32_t SetId, ref<egxsetpool> Pool)
{
	return { new egxshaderset(CoreInterface, SetId, Pool) };
}

egx::ref<egx::egxshaderset>  egx::egxshaderset::FactoryCreate(ref<VulkanCoreInterface>& CoreInterface, uint32_t SetId)
{
	return FactoryCreate(CoreInterface, SetId, egxsetpool::FactoryCreate(CoreInterface));
}

egx::egxshaderset::egxshaderset(egxshaderset&& move) noexcept
	: SetId(move.SetId), _coreinterface(move._coreinterface)
{
	this->~egxshaderset();
	memcpy(this, &move, sizeof(egxshaderset));
	memset(&move, 0, sizeof(egxshaderset));
}

egx::egxshaderset& egx::egxshaderset::operator=(egxshaderset& move) noexcept
{
	this->~egxshaderset();
	memcpy(this, &move, sizeof(egxshaderset));
	memset(&move, 0, sizeof(egxshaderset));
	return *this;
}

egx::egxshaderset::~egxshaderset() noexcept
{
	if (_setlayout) {
		vkDestroyDescriptorSetLayout(_coreinterface->Device, _setlayout, nullptr);
	}
}

void egx::egxshaderset::CreateBuffer(
	uint32_t BindingId,
	VkShaderStageFlags Stages,
	VkDescriptorType Type,
	memorylayout Layout,
	uint32_t Size,
	uint32_t BlockSize)
{
	assert(_images.find(BindingId) == _images.end() && _buffers.find(BindingId) == _buffers.end());
	_internal::bufferinformation info{};
	info.BindingId = BindingId;
	info.Stages = Stages;
	info.Type = Type;
	info.Layout = Layout;
	info.BlockSize = BlockSize == 0 ? Size : BlockSize;
	info.Size = Size;
	_buffers.insert({ BindingId, info });
	_pool->AddType(Type, 1);
	if (Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
		DynamicOffsets[BindingId] = 0;
	}
}

void egx::egxshaderset::AddBuffer(
	uint32_t BindingId,
	VkShaderStageFlags Stages,
	VkDescriptorType Type,
	ref<Buffer> buffer,
	uint32_t BlockSize)
{
	assert(buffer.IsValidRef() && _images.find(BindingId) == _images.end() && _buffers.find(BindingId) == _buffers.end());
	_internal::bufferinformation info{};
	info.BindingId = BindingId;
	info.Stages = Stages;
	info.Type = Type;
	info.Layout = buffer->Layout;
	info.Size = buffer->Size;
	info.BlockSize = BlockSize == 0 ? buffer->Size : BlockSize;
	info.Buffer = buffer;
	_buffers.insert({ BindingId, info });
	_pool->AddType(Type, 1);
	if (Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
		DynamicOffsets[BindingId] = 0;
	}
}

void egx::egxshaderset::AddImage(
	uint32_t BindingId,
	VkShaderStageFlags Stages,
	VkDescriptorType Type,
	VkSampler Sampler,
	ref<Image> Image,
	VkImageLayout ImageLayout)
{
	this->AddImageArray(BindingId, Stages, Type, Sampler, { Image }, { ImageLayout });
}

void egx::egxshaderset::AddImageArray(
	uint32_t BindingId,
	VkShaderStageFlags Stages,
	VkDescriptorType Type,
	VkSampler Sampler,
	const std::vector<ref<Image>>& Images,
	VkImageLayout ImageLayout)
{
	std::vector<VkImageLayout> layouts(Images.size());
	for (int i = 0; i < Images.size(); i++) {
		layouts[i] = ImageLayout;
	}
	this->AddImageArray(BindingId, Stages, Type, Sampler, Images, layouts);
}

void egx::egxshaderset::AddImageArray(
	uint32_t BindingId,
	VkShaderStageFlags Stages,
	VkDescriptorType Type,
	VkSampler Sampler,
	const std::vector<ref<Image>>& Images,
	std::vector<VkImageLayout> ImageLayouts)
{
	assert(Images.size() == ImageLayouts.size() && _images.find(BindingId) == _images.end() && _buffers.find(BindingId) == _buffers.end());
	_images[BindingId].BindingId = BindingId;
	_images[BindingId].Stages = Stages;
	_images[BindingId].Type = Type;
	_images[BindingId].Sampler = Sampler;
	for (int i = 0; i < Images.size(); i++) {
		_images[BindingId].Image.push_back(Images[i]);
		_images[BindingId].ImageLayout.push_back(ImageLayouts[i]);
		_pool->AddType(Type, 1);
	}
}

VkDescriptorSet& egx::egxshaderset::GetSet()
{
	if (_set) return _set;
	// 1) Create buffers
	for (auto& [i, b] : _buffers) {
		if (!b.Buffer.IsValidRef()) {
			if (b.Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || b.Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
				b.Buffer = egx::Buffer::FactoryCreate(_coreinterface, b.Size, b.Layout, buffertype_uniform);
			else
				b.Buffer = egx::Buffer::FactoryCreate(_coreinterface, b.Size, b.Layout, buffertype_storage);
		}
		Buffers[i] = b.Buffer;
	}
	// 3) Create pool
	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = _pool->getpool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &GetSetLayout();
	vkAllocateDescriptorSets(_coreinterface->Device, &allocInfo, &_set);
	// 4) Write buffer and images to set
	{
		int i = 0;
		std::vector<ref<VkDescriptorBufferInfo>> BufferInfos;
		std::vector<VkDescriptorImageInfo> ImageInfos;
		std::vector<VkWriteDescriptorSet> Writes(_buffers.size() + _images.size());
		for (auto& [Id, info] : _buffers) {
			Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			Writes[i].pNext = nullptr;
			Writes[i].dstSet = _set;
			Writes[i].dstBinding = Id;
			Writes[i].dstArrayElement = 0;
			Writes[i].descriptorCount = 1;
			Writes[i].descriptorType = info.Type;
			Writes[i].pImageInfo = nullptr;

			ref<VkDescriptorBufferInfo> bufferInfo(new VkDescriptorBufferInfo);
			bufferInfo->buffer = info.Buffer->Buf;
			bufferInfo->offset = 0;
			bufferInfo->range = info.BlockSize;
			BufferInfos.push_back(bufferInfo);

			Writes[i].pBufferInfo = bufferInfo();
			Writes[i].pTexelBufferView = nullptr;
			i++;
		}

		for (auto& [Id, info] : _images) {
			Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			Writes[i].pNext = nullptr;
			Writes[i].dstSet = _set;
			Writes[i].dstBinding = Id;
			Writes[i].dstArrayElement = 0;
			Writes[i].descriptorCount = (uint32_t)info.Image.size();
			Writes[i].descriptorType = info.Type;

			int j = ImageInfos.size();
			for (int k = 0; k < info.Image.size(); k++) {
				VkDescriptorImageInfo imageInfo{};
				imageInfo.sampler = info.Sampler;
				imageInfo.imageView = info.Image[k]->view(0);
				imageInfo.imageLayout = info.ImageLayout[k];
				ImageInfos.push_back(imageInfo);
			}

			Writes[i].pImageInfo = &ImageInfos[j];
			Writes[i].pBufferInfo = nullptr;
			Writes[i].pTexelBufferView = nullptr;
			i++;
		}

		vkUpdateDescriptorSets(_coreinterface->Device, Writes.size(), Writes.data(), 0, nullptr);
	}
	return _set;
}

VkDescriptorSetLayout& egx::egxshaderset::GetSetLayout()
{
	// 2) Create Set layout
	if (_setlayout) return _setlayout;
	int i = 0;
	VkDescriptorSetLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	std::vector<VkDescriptorSetLayoutBinding> bindings(_buffers.size() + _images.size());
	for (auto& [Id, info] : _buffers) {
		bindings[i].binding = info.BindingId;
		bindings[i].descriptorType = info.Type;
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags = info.Stages;
		bindings[i].pImmutableSamplers = nullptr;
		i++;
	}
	for (auto& [Id, info] : _images) {
		bindings[i].binding = info.BindingId;
		bindings[i].descriptorType = info.Type;
		bindings[i].descriptorCount = info.Image.size();
		bindings[i].stageFlags = info.Stages;
		bindings[i].pImmutableSamplers = nullptr;
		i++;
	}
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	vkCreateDescriptorSetLayout(_coreinterface->Device, &createInfo, nullptr, &_setlayout);
	return _setlayout;
}

#pragma endregion

#pragma region set pool
egx::ref<egx::egxsetpool> egx::egxsetpool::FactoryCreate(ref<VulkanCoreInterface>& CoreInterface)
{
	return ref<egxsetpool>(new egxsetpool(CoreInterface));
}

egx::egxsetpool::egxsetpool(egxsetpool&& move) noexcept
	: _coreinterface(move._coreinterface)
{
	this->~egxsetpool();
	memcpy(&move, this, sizeof(egxsetpool));
	memset(&move, 0, sizeof(egxsetpool));
}

egx::egxsetpool& egx::egxsetpool::operator=(egxsetpool& move) noexcept
{
	this->~egxsetpool();
	memcpy(&move, this, sizeof(egxsetpool));
	memset(&move, 0, sizeof(egxsetpool));
	return *this;
}

egx::egxsetpool::~egxsetpool()
{
	if (_pool)
		vkDestroyDescriptorPool(_coreinterface->Device, _pool, nullptr);
}

void egx::egxsetpool::IncrementSetCount()
{
	assert(_pool == nullptr);
	this->_setcount++;
}

void egx::egxsetpool::AddType(VkDescriptorType type, uint32_t count)
{
	assert(_pool == nullptr);
	this->_types[type] += count;
}

VkDescriptorPool egx::egxsetpool::getpool()
{
	if (_pool != nullptr) return _pool;
	VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

	int i = 0;
	std::vector<VkDescriptorPoolSize> sizes(_types.size());
	for (auto& [type, count] : _types) {
		sizes[i].type = type;
		sizes[i].descriptorCount = count;
		i++;
	}

	createInfo.flags = 0;
	createInfo.maxSets = _setcount;
	createInfo.poolSizeCount = sizes.size();
	createInfo.pPoolSizes = sizes.data();
	vkCreateDescriptorPool(_coreinterface->Device, &createInfo, nullptr, &_pool);
	return _pool;
}
#pragma endregion

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

void egx::PipelineLayout::AddSet(ref<egxshaderset>& set)
{
	assert(_layout == nullptr);
	Sets.insert({ set->SetId, set });
}

void egx::PipelineLayout::AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages)
{
	assert(_layout == nullptr);
	_ranges.push_back({ stages, offset, size });
}

VkPipelineLayout& egx::PipelineLayout::GetLayout()
{
	if (_layout) return _layout;
	std::vector<VkDescriptorSetLayout> setLayouts(Sets.size());
	for (int i = 0; i < setLayouts.size(); i++) {
		setLayouts[i] = Sets[i]->GetSetLayout();
	}
	VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	createInfo.setLayoutCount = setLayouts.size();
	createInfo.pSetLayouts = setLayouts.data();
	createInfo.pushConstantRangeCount = (uint32_t)_ranges.size();
	createInfo.pPushConstantRanges = _ranges.data();
	vkCreatePipelineLayout(_coreinterface->Device, &createInfo, nullptr, &_layout);
	// Cache data from set structure
	_cached_set.resize(Sets.size());
	for (int i = 0; i < _cached_set.size(); i++) {
		_cached_set[i] = Sets[i]->GetSet();
	}
	for (auto& [id, set] : Sets) {
		_cached_dynamicoffsets.resize(_cached_dynamicoffsets.size() + set->DynamicOffsets.size());
	}
	this->UpdateDynamicOffsets();
	return _layout;
}

#pragma endregion
