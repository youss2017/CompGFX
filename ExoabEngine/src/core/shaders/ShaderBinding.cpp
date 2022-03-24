#include "ShaderBinding.hpp"

ShaderSet ShaderBinding_Create(vk::VkContext context, VkDescriptorPool pool, uint32_t setID, std::vector<ShaderBinding>* shader_bindings)
{
    ShaderSet set = new _ShaderSet();
    auto framecount = gFrameOverlapCount;
    // 1) Create setlayout
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<IBuffer2> buffers;
    for (auto& bind : *shader_bindings)
    {
        VkDescriptorSetLayoutBinding binding;
        binding.binding = bind.m_bindingID;
        binding.descriptorCount = (bind.m_type > SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT_DYNAMIC) ? bind.m_textures.size() : 1;
        binding.stageFlags = bind.m_shaderStages;
        binding.pImmutableSamplers = nullptr;
        switch (bind.m_type)
        {
            case SHADER_BINDING_UNIFORM_BUFFER:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                if (!bind.m_useclientbuffer && !bind.m_preinitalized) {
                    bind.m_buffer = new IBuffer2[framecount];
                    for (int32_t i = 0; i < framecount; i++)
                        bind.m_buffer[i] = Buffer2_Create((BufferType)(BUFFER_TYPE_UNIFORM | bind.m_additional_buffer_flags), bind.m_size, bind.m_hostvisible ? BufferMemoryType::CPU_TO_CPU : BufferMemoryType::GPU_ONLY, true, false, false);
                }
                bind.m_vk_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case SHADER_BINDING_UNIFORM_BUFFER_DYNAMIC:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                if (!bind.m_useclientbuffer && !bind.m_preinitalized) {
                    bind.m_buffer = new IBuffer2[framecount];
                    for (int32_t i = 0; i < framecount; i++)
                        bind.m_buffer[i] = Buffer2_Create((BufferType)(BUFFER_TYPE_UNIFORM| bind.m_additional_buffer_flags), bind.m_size, bind.m_hostvisible ? BufferMemoryType::CPU_TO_CPU : BufferMemoryType::GPU_ONLY, true, false, false);
                }
                bind.m_vk_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                break;
            case SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                if (!bind.m_useclientbuffer && !bind.m_preinitalized) {
                    bind.m_buffer = new IBuffer2[framecount];
                    for (int32_t i = 0; i < framecount; i++)
                        bind.m_buffer[i] = Buffer2_Create((BufferType)(BUFFER_TYPE_STORAGE | bind.m_additional_buffer_flags), bind.m_size, bind.m_hostvisible ? BufferMemoryType::CPU_TO_CPU : BufferMemoryType::GPU_ONLY, true, false, false);
                }
                bind.m_vk_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            case SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT_DYNAMIC:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                if (!bind.m_useclientbuffer && !bind.m_preinitalized) {
                    bind.m_buffer = new IBuffer2[framecount];
                    for (int32_t i = 0; i < framecount; i++)
                        bind.m_buffer[i] = Buffer2_Create((BufferType)(BUFFER_TYPE_STORAGE | bind.m_additional_buffer_flags), bind.m_size, bind.m_hostvisible ? BufferMemoryType::CPU_TO_CPU : BufferMemoryType::GPU_ONLY, true, false, false);
                }
                bind.m_vk_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                break;
            case SHADER_BINDING_COMBINED_TEXTURE_SAMPLER:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                bind.m_vk_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
             case SHADER_BINDING_TEXTURE:
                 binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                 bind.m_vk_type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                 break;
             case SHADER_BINDING_SAMPLER:
                 binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                 bind.m_vk_type = VK_DESCRIPTOR_TYPE_SAMPLER;
                 break;
             case SHADER_BINDING_STORAGE_IMAGE:
                 // Cannot have both mipview chain and chain of texture views in same binding.
                 binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                 binding.descriptorCount = bind.mUseViewPerMip ? bind.m_textures[0]->mMipCount : binding.descriptorCount;
                 bind.m_vk_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                 break;
            default:
                assert(0);
        }
        bindings.push_back(binding);
    }
    set->m_bindings.resize(shader_bindings->size());
    for (int i = 0; i < shader_bindings->size(); i++)
        set->m_bindings[i] = (*shader_bindings)[i];
    VkDescriptorSetLayoutCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();
    VkDescriptorSetLayout setlayout;
    vkcheck(vkCreateDescriptorSetLayout(context->defaultDevice, &createInfo, nullptr, &setlayout));
    // 2) Allocate descriptor sets
    VkDescriptorSetAllocateInfo allocateInfo;
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.descriptorPool = pool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &setlayout;
    set->m_set = new VkDescriptorSet[framecount];
    for(int32_t i = 0; i < framecount; i++)
        vkcheck(vkAllocateDescriptorSets(context->defaultDevice, &allocateInfo, &set->m_set[i]));
    // 3) Write to sets
    for (int32_t i = 0; i < framecount; i++)
    {
        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = set->m_set[i];
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.pImageInfo = nullptr;
        write.pTexelBufferView = nullptr;
        write.pBufferInfo = nullptr;
        for (auto& bind : set->m_bindings) {
            write.dstBinding = bind.m_bindingID;
            write.descriptorType = bind.m_vk_type;
            if (bind.m_type != SHADER_BINDING_COMBINED_TEXTURE_SAMPLER && bind.m_type != SHADER_BINDING_TEXTURE && bind.m_type != SHADER_BINDING_SAMPLER && bind.m_type != SHADER_BINDING_STORAGE_IMAGE) {
                VkDescriptorBufferInfo bufferInfo;
                bufferInfo.buffer = (bind.m_useclientbuffer) ? bind.m_client_buffer->mBuffer : bind.m_buffer[i]->mBuffer;
                bufferInfo.offset = 0;
                bufferInfo.range = VK_WHOLE_SIZE;
                write.pBufferInfo = &bufferInfo;
                vkUpdateDescriptorSets(context->defaultDevice, 1, &write, 0, nullptr);
            }
            else {
                if (bind.m_type == SHADER_BINDING_COMBINED_TEXTURE_SAMPLER || bind.m_type == SHADER_BINDING_STORAGE_IMAGE) {
                    write.dstArrayElement = 0;
                    write.descriptorCount = bind.m_textures.size();
                    std::vector<VkDescriptorImageInfo> imageInfos;
                    assert(bind.m_textures.size() == bind.m_textures_layouts.size());
                    for (int w = 0; w < bind.m_textures.size(); w++)
                    {
                        VkDescriptorImageInfo imageInfo;
                        imageInfo.sampler = bind.m_sampler[w];
                        ITexture2 texture = bind.m_textures[w];
                        imageInfo.imageLayout = bind.m_textures_layouts[w];
                        if (!bind.mUseViewPerMip) {
                            if (bind.m_textures[w]->m_vk_views_per_frame.size() > 0)
                                imageInfo.imageView = texture->m_vk_views_per_frame[i];
                            else
                                imageInfo.imageView = texture->m_vk_view;
                            imageInfos.push_back(imageInfo);
                            write.pImageInfo = imageInfos.data();
                            vkUpdateDescriptorSets(context->defaultDevice, 1, &write, 0, nullptr);
                        }
                        else {
                            write.descriptorCount = 1;
                            int arrayElement = 0;
                            auto& views = texture->mMipmapViews.mMipmapViewsPerFrame[i];
                            for (auto& view : views) {
                                imageInfo.imageView = view;
                                write.dstArrayElement = arrayElement++;
                                write.pImageInfo = &imageInfo;
                                vkUpdateDescriptorSets(context->defaultDevice, 1, &write, 0, nullptr);
                            }
                        }
                    }
                } else if (bind.m_type == SHADER_BINDING_SAMPLER) {
                    assert(!bind.mUseViewPerMip);
                    write.dstArrayElement = 0;
                    write.descriptorCount = bind.m_sampler.size();
                    std::vector<VkDescriptorImageInfo> imageInfos(bind.m_sampler.size());
                    for (int w = 0; w < bind.m_sampler.size(); w++)
                    {
                        imageInfos[w].sampler = bind.m_sampler[w];
                        imageInfos[w].imageView = nullptr;
                        imageInfos[w].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    }
                    write.pImageInfo = imageInfos.data();
                    vkUpdateDescriptorSets(context->defaultDevice, 1, &write, 0, nullptr);
                }
                else if (bind.m_type == SHADER_BINDING_TEXTURE) {
                    assert(!bind.mUseViewPerMip);
                    assert(bind.m_sampler.size() == 0 && "Combined samplers cannot have samplers!");
                    write.dstArrayElement = 0;
                    write.descriptorCount = bind.m_textures.size();
                    std::vector<VkDescriptorImageInfo> imageInfos(bind.m_textures.size());
                    assert(bind.m_textures.size() == bind.m_textures_layouts.size());
                    for (int w = 0; w < bind.m_textures.size(); w++)
                    {
                        assert(bind.m_textures[w]->m_vk_views_per_frame.size() == 0);
                        imageInfos[w].sampler = nullptr;
                        imageInfos[w].imageView = bind.m_textures[w]->m_vk_view;
                        imageInfos[w].imageLayout = bind.m_textures_layouts[w];
                    }
                    write.pImageInfo = imageInfos.data();
                    vkUpdateDescriptorSets(context->defaultDevice, 1, &write, 0, nullptr);
                }
            }
        }
    }
    set->m_setlayout = setlayout;
    return set;
}

void ShaderBinding_DestroySets(vk::VkContext context, std::vector<ShaderSet> sets)
{
    for (auto& set : sets)
    {
        for (auto& binding : set->m_bindings)
        {
            if (binding.m_textures.size() > 0 || binding.m_useclientbuffer)
                continue;
            if(!binding.m_preinitalized)
                for(unsigned int q = 0; q < gFrameOverlapCount; q++)
                    Buffer2_Destroy(binding.m_buffer[q]);
        }
        vkDestroyDescriptorSetLayout(context->defaultDevice, set->m_setlayout, nullptr);
        delete set;
    }
}

VkPipelineLayout ShaderBinding_CreatePipelineLayout(vk::VkContext context, std::vector<ShaderSet> sets, std::vector<VkPushConstantRange> ranges)
{
    std::vector<VkDescriptorSetLayout> setLayouts;
    for (auto& set : sets) setLayouts.push_back(set->m_setlayout);
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

void ShaderBinding_CalculatePoolSizes(uint32_t framecount, std::vector<VkDescriptorPoolSize>& poolSizes, const std::vector<ShaderBinding>* bindings)
{
    for (auto& bind : *bindings) {
        switch (bind.m_type)
        {
        case SHADER_BINDING_UNIFORM_BUFFER:
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framecount });
            break;
        case SHADER_BINDING_UNIFORM_BUFFER_DYNAMIC:
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, framecount });
            break;
        case SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT:
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, framecount });
            break;
        case SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT_DYNAMIC:
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, framecount });
            break;
        case SHADER_BINDING_COMBINED_TEXTURE_SAMPLER:
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, framecount * 2 });
            break;
        case SHADER_BINDING_TEXTURE:
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, framecount });
            break;
        case SHADER_BINDING_SAMPLER:
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, framecount });
            break;
        case SHADER_BINDING_STORAGE_IMAGE:
            poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, uint32_t(framecount * bind.m_textures[0]->mMipmapViews.mMipmapViewsPerFrame[0].size()) });
            break;
        default:
            assert(0);
        }
    }
}
