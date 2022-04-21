#include "ShaderConnector.hpp"
#include <backend/VkGraphicsCard.hpp>

void GRAPHICS_API ShaderConnector_CalculateDescriptorPool(uint32_t bindingsCount, BindingDescription* pBindings, std::vector<VkDescriptorPoolSize>& poolSizes) {
    for (uint32_t i = 0; i < bindingsCount; i++) {
        auto& binding = pBindings[i];
        unsigned int size = 0;
        switch (binding.mType)
        {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            poolSizes.push_back({ binding.mType, gFrameOverlapCount });
            break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            for (auto& texture : binding.mTextures) {
                if (texture->m_specification.mCreateViewPerMip) {
                    size += texture->mMipCount * gFrameOverlapCount;
                }
                else {
                    size++;
                }
            }
            poolSizes.push_back({ binding.mType, size * gFrameOverlapCount });
            break;
        default:
            assert(0);
        }
    }
}

DescriptorSet GRAPHICS_API ShaderConnector_CreateSet(vk::VkContext context, uint32_t setID, VkDescriptorPool pool, uint32_t bindingsCount, BindingDescription* pBindings) {
    DescriptorSet set;
    set.mSetID = setID;
    set.mPool = pool;

    for (uint32_t i = 0; i < bindingsCount; i++) {
        BindingDescription& binding = pBindings[i];
        if (binding.mBuffer)
            binding.mSharedResources = true;
        if (binding.mSharedResources && binding.mBuffer == nullptr) {
            logwarning("Shared resource without providing the buffer!");
        }
        auto type = binding.mType;
        if ((type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) || (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)) {
            set.mBindings.insert(std::make_pair(binding.mBindingID, binding));
        }
        else {
            if (binding.mBuffer == nullptr) {
                BufferType bufferType;
                if ((type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) || (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC))
                    bufferType = BUFFER_TYPE_UNIFORM;
                else if ((type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) || (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC))
                    bufferType = BUFFER_TYPE_STORAGE;
                else {
                    assert(0);
                }
                if (binding.mFlags & BINDING_FLAG_BUFFER_USAGE_INDIRECT)
                    bufferType = BufferType(bufferType | BUFFER_TYPE_INDIRECT);
                BindingFlags flags = binding.mFlags;
                BufferMemoryType memoryType = (binding.mFlags & BINDING_FLAG_CPU_VISIBLE) ? BufferMemoryType::CPU_TO_GPU : BufferMemoryType::GPU_ONLY;
                binding.mBuffer = Buffer2_Create(context, bufferType, binding.mBufferSize, memoryType, (binding.mFlags & BINDING_FLAG_CREATE_BUFFERS_POINTER), (binding.mFlags & BINDING_FLAG_REQUIRE_COHERENT));
            }
            set.mBindings.insert(std::make_pair(binding.mBindingID, binding));
        }
    }

    std::vector<VkDescriptorSetLayoutBinding> setLayoutDescs;
    for (const auto& bindingPair : set.mBindings) {
        auto& binding = bindingPair.second;
        VkDescriptorSetLayoutBinding setLayoutDesc{};
        setLayoutDesc.binding = binding.mBindingID;
        setLayoutDesc.descriptorType = binding.mType;
        auto type = binding.mType;
        if ((type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) || (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)) {
            setLayoutDesc.descriptorCount = 0;
            for (auto& texture : binding.mTextures) {
                if (texture->m_specification.mCreateViewPerMip && !(binding.mFlags & BINDING_FLAG_USING_ONLY_FIRST_MIP)) {
                    setLayoutDesc.descriptorCount += texture->mMipCount;
                }
                else {
                    setLayoutDesc.descriptorCount++;
                }
            }
        }
        else
            setLayoutDesc.descriptorCount = 1;
        setLayoutDesc.stageFlags = binding.mStages;
        setLayoutDescs.push_back(setLayoutDesc);
    }

    VkDescriptorSetLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    createInfo.flags = 0;
    createInfo.bindingCount = setLayoutDescs.size();
    createInfo.pBindings = setLayoutDescs.data();
    VkDescriptorSetLayout setlayout;
    vkcheck(vkCreateDescriptorSetLayout(context->defaultDevice, &createInfo, nullptr, &setlayout));
    set.mSetLayout = setlayout;

    VkDescriptorSetAllocateInfo allocateInfo;
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.descriptorPool = pool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &setlayout;
    for (int32_t i = 0; i < gFrameOverlapCount; i++)
        vkcheck(vkAllocateDescriptorSets(context->defaultDevice, &allocateInfo, &set.mSets[i]));

    for (uint32_t frameIndex = 0; frameIndex < gFrameOverlapCount; frameIndex++) {
        for (auto& bindingPair : set.mBindings) {
            auto& binding = bindingPair.second;
            auto type = binding.mType;
            if ((type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) || (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)) {
                VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                write.dstSet = set[frameIndex];
                write.dstBinding = binding.mBindingID;
                write.descriptorType = type;
                write.dstArrayElement = 0;
                write.descriptorCount = 1;
                for (int textureID = 0; textureID < binding.mTextures.size(); textureID++) {
                    auto& texture = binding.mTextures[textureID];
                    VkDescriptorImageInfo imageInfo;
                    imageInfo.sampler = binding.mSampler;
                    imageInfo.imageLayout = (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
                    if (binding.mCustomImageLayouts.find(textureID) != binding.mCustomImageLayouts.end()) {
                        imageInfo.imageLayout = binding.mCustomImageLayouts[textureID];
                    }
                    if (texture->m_specification.mCreateViewPerMip && !(binding.mFlags & BINDING_FLAG_USING_ONLY_FIRST_MIP)) {
                        for (uint32_t mipIndex = 0; mipIndex < texture->mMipCount; mipIndex++) {
                            imageInfo.imageView = texture->mMipmapViews.mMipmapViewsPerFrame[frameIndex][mipIndex];
                            write.pImageInfo = &imageInfo;
                            vkUpdateDescriptorSets(context->defaultDevice, 1, &write, 0, nullptr);
                            write.dstArrayElement++;
                        }
                    }
                    else {
                        if (texture->m_vk_views_per_frame.size() == gFrameOverlapCount) {
                            imageInfo.imageView = texture->m_vk_views_per_frame[frameIndex];
                        }
                        else {
                            imageInfo.imageView = texture->m_vk_view;
                        }
                        write.pImageInfo = &imageInfo;
                        vkUpdateDescriptorSets(context->defaultDevice, 1, &write, 0, nullptr);
                        write.dstArrayElement++;
                    }
                }
            } else {
                VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                write.dstSet = set[frameIndex];
                write.dstBinding = binding.mBindingID;
                write.descriptorType = type;
                write.dstArrayElement = 0;
                write.descriptorCount = 1;
                VkDescriptorBufferInfo bufferInfo;
                bufferInfo.buffer = binding.mBuffer->mBuffers[frameIndex];
                bufferInfo.offset = 0;
                bufferInfo.range = VK_WHOLE_SIZE;
                write.pBufferInfo = &bufferInfo;
                vkUpdateDescriptorSets(context->defaultDevice, 1, &write, 0, nullptr);
            }
        }
    }

    return set;
}

VkPipelineLayout GRAPHICS_API ShaderConnector_CreatePipelineLayout(vk::VkContext context, uint32_t descriptorSetCount, DescriptorSet* pSets, std::vector<VkPushConstantRange> ranges) {
    std::vector<VkDescriptorSetLayout> setLayouts;
    for (uint32_t i = 0; i < descriptorSetCount; i++) setLayouts.push_back(pSets[i].mSetLayout);
    VkPipelineLayoutCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.setLayoutCount = setLayouts.size();
    createInfo.pSetLayouts = setLayouts.data();
    createInfo.pushConstantRangeCount = ranges.size();
    createInfo.pPushConstantRanges = ranges.data();
    VkPipelineLayout layout;
    vkcheck(vkCreatePipelineLayout(context->defaultDevice, &createInfo, nullptr, &layout));
    return layout;
}

void GRAPHICS_API ShaderConnector_DestroySet(VkDevice device, const DescriptorSet& set) {
    vkDestroyDescriptorSetLayout(device, set.mSetLayout, nullptr);
    //vkFreeDescriptorSets(Global::Context->defaultDevice, set.mPool, gFrameOverlapCount, &set.mSets[0]);
    for (auto& bindingPair : set.mBindings) {
        auto& binding = bindingPair.second;
        if (binding.mSharedResources)
            continue;
        auto type = binding.mType;
        if ((type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) || (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE))
            continue;
        Buffer2_Destroy(binding.mBuffer);
    }
}
