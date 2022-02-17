#include "ShaderProgramData.hpp"
#include "Material.hpp"

#include "ShaderProgramData_Internal.inl"

static void Vulkan_Internal_InitalizeShaderProgramData(GraphicsContext context, IShaderProgramData shaderDataLayout, IPipelineLayout layout);
static void Vulkan_Internal_ExpandDescriptorSetBuffer(vk::VkContext context, ShaderProgramDataReserved* reserved, IPipelineLayout layout, DynamicSetInformation& setInformation, int bindingID, int AdditionalUseCount);

IShaderProgramData ShaderProgramData_Create(IPipelineLayout layout)
{
	ShaderProgramData *sgd = new ShaderProgramData();
	sgd->m_context = layout->m_context;
	sgd->m_vertex_reflection = layout->m_vertex_reflection;
	sgd->m_fragment_reflection = layout->m_fragment_reflection;
	sgd->m_combined_reflection = ShaderReflection::CombineReflections(&sgd->m_vertex_reflection, &sgd->m_fragment_reflection);
	sgd->m_reserved = nullptr;
	return sgd;
}

void ShaderProgramData_SetConstantTextureArray(IShaderProgramData programData, int setID, int bindingID, IGPUTextureSampler sampler, const std::vector<IGPUTexture2D> &textures)
{
	assert(sampler);
	if (programData->m_textures.size() > 0)
		assert(programData->m_textures.size() == textures.size());
	programData->m_sampler = sampler;
	programData->m_textures = textures;
	programData->m_textures_updated = true;
	programData->m_texture_setID = setID;
	programData->m_texture_bindingID = bindingID;
}

void ShaderProgramData_SetConstantSSBO(IShaderProgramData programData, int setID, int bindingID, IBuffer2 ssbo_buffer)
{
    SSBOInformation i;
    i.m_setID = setID, i.m_bindingID = bindingID, i.m_ssbobuffer = ssbo_buffer;
    programData->m_ssbos.push_back(i);
}

void ShaderProgramData_UpdateBindingData(IShaderProgramData program_data, IPipelineLayout layout, uint32_t count, EntityBindingData *bindingData)
{
	PROFILE_FUNCTION();
	if (!program_data->m_reserved)
	{
		Vulkan_Internal_InitalizeShaderProgramData(program_data->m_context, program_data, layout);
	}

	ShaderProgramDataReserved *dynamicInfo = (ShaderProgramDataReserved *)program_data->m_reserved;
	vk::VkContext c = ToVKContext(program_data->m_context);

	bool FrameChanged = c->FrameInfo->m_FrameIndex != dynamicInfo->lastFrameIndex;
	for (auto &setInfo : dynamicInfo->setInformations)
		for (auto &uni : setInfo.uniformInformation)
			uni.second.FrameChanged = FrameChanged;
	dynamicInfo->lastFrameIndex = c->FrameInfo->m_FrameIndex;

	for (uint32_t dataID = 0; dataID < count; dataID++)
	{
		EntityBindingData &data = bindingData[dataID];
		if (data.bindingData.size() == 0)
			continue;
		auto &setInfo = dynamicInfo->setInformations[data.setID];
		auto &uniformInfo = setInfo.uniformInformation[data.bindingID];
		char *ptr = (char *)Buffer2_Map(setInfo.buffer, false, true);
		if (uniformInfo.FrameChanged)
			uniformInfo.usedCopies = 0;
		uniformInfo.startUsedCopies = uniformInfo.usedCopies;
		if (uniformInfo.usedCopies >= uniformInfo.copyCount)
		{
			Vulkan_Internal_ExpandDescriptorSetBuffer(c, dynamicInfo, layout, setInfo, data.bindingID, 5);
		}
		for (int elementID = 0; elementID < data.size(); elementID++)
		{
			auto &t = data[elementID];
			char *sub_ptr = ptr + (uniformInfo.offset + (uniformInfo.bufferSize * uniformInfo.usedCopies)) + t.elementOffsetInStruct;
			memcpy(sub_ptr, &t.dataunion, t.elementSize);
		}
		uniformInfo.FrameChanged = false;
		uniformInfo.usedCopies++;
	}
}

void ShaderProgramData_UpdateEntityBindingData(IShaderProgramData program_data, IPipelineLayout layout, uint32_t count, void **_entities)
{
	PROFILE_FUNCTION();
    if (!program_data->m_reserved)
    {
        Vulkan_Internal_InitalizeShaderProgramData(program_data->m_context, program_data, layout);
    }
    ShaderProgramDataReserved *dynamicInfo = (ShaderProgramDataReserved *)program_data->m_reserved;
    vk::VkContext c = ToVKContext(program_data->m_context);

    bool FrameChanged = c->FrameInfo->m_FrameIndex != dynamicInfo->lastFrameIndex;
    for (auto &setInfo : dynamicInfo->setInformations)
        for (auto &uni : setInfo.uniformInformation)
            uni.second.FrameChanged = FrameChanged;
    dynamicInfo->lastFrameIndex = c->FrameInfo->m_FrameIndex;

    DefaultEntity **entities = (DefaultEntity **)_entities;
    // Upload Data All at once
    {
        PROFILE_SCOPE("Copying Data to Driver Memory.");
        for (uint32_t entityID = 0; entityID < count; entityID++)
        {
            DefaultEntity *entity = entities[entityID];
            for (int dataID = 0; dataID < entity->size(); dataID++)
            {
                EntityBindingData &data = entity->get(dataID);
                if (data.bindingData.size() == 0)
                    continue;
                auto &setInfo = dynamicInfo->setInformations[data.setID];
                auto &uniformInfo = setInfo.uniformInformation[data.bindingID];
                char *ptr = (char *)Buffer2_Map(setInfo.buffer, false, true);
                if (uniformInfo.FrameChanged)
                    uniformInfo.usedCopies = 0;
                uniformInfo.startUsedCopies = uniformInfo.usedCopies;
                if (uniformInfo.usedCopies >= uniformInfo.copyCount)
                {
                    Vulkan_Internal_ExpandDescriptorSetBuffer(c, dynamicInfo, layout, setInfo, data.bindingID, 5);
                }
                for (int elementID = 0; elementID < data.size(); elementID++)
                {
                    auto &t = data[elementID];
                    char *sub_ptr = ptr + (uniformInfo.offset + (uniformInfo.bufferSize * uniformInfo.usedCopies)) + t.elementOffsetInStruct;
                    memcpy(sub_ptr, &t.dataunion, t.elementSize);
                }
                uniformInfo.FrameChanged = false;
                uniformInfo.usedCopies++;
            }
        }
    }
}

void ShaderProgramData_FlushShaderProgramData(uint32_t count, IShaderProgramData* pShaderDatas)
{
    PROFILE_FUNCTION();
    for (uint32_t i = 0; i < count; i++)
    {
        IShaderProgramData shader_data = pShaderDatas[i];
        ShaderProgramDataReserved* reserved = (ShaderProgramDataReserved*)shader_data->m_reserved;
        for (const auto& setInfo : reserved->setInformations)
        {
            if (setInfo.isBuffer)
                Buffer2_Flush(setInfo.buffer);
        }
    }
}

static void Vulkan_Internal_InitalizeShaderProgramData(GraphicsContext context, IShaderProgramData programData, IPipelineLayout layout)
{
    PROFILE_FUNCTION();
    ShaderProgramDataReserved* reserved = new ShaderProgramDataReserved();
    programData->m_reserved = reserved;

    reserved->context = ToVKContext(context);
    reserved->lastFrameIndex = vk::GetCurrentFrameIndex(context);

    reserved->descriptorSetCount = layout->m_real_setlayouts.size();
    reserved->pDescriptorSets = new VkDescriptorSet[layout->m_real_setlayouts.size()];

    vk::VkContext c = ToVKContext(context);
    reserved->descriptorPool = vk::Gfx_CreateDescriptorPool(c, reserved->descriptorSetCount,
        { {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100} });

    // TODO: This may not work if there is gaps between sets, eg layout(set = 0) --- layout(set = 3) idk

    VkDescriptorSetAllocateInfo allocateInfo;
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.descriptorPool = reserved->descriptorPool;
    allocateInfo.descriptorSetCount = layout->m_real_setlayouts.size();
    allocateInfo.pSetLayouts = layout->m_real_setlayouts.data();
    vkcheck_break(vkAllocateDescriptorSets(c->defaultDevice, &allocateInfo, reserved->pDescriptorSets));

    // Create IGPUBuffer per descripter set
    const std::vector<DescriptorSetDescription>& setDescriptions = programData->m_combined_reflection.GetShaderSetDescriptions();
    int index = 0;
    for (auto& setDescription : setDescriptions)
    {
        if (setDescription.m_UniformBuffers.size() > 0)
        {
            size_t totalSize = 0;
            DynamicSetInformation setInformation;
            setInformation.setHandleIndex = index;
            setInformation.setHandle = reserved->pDescriptorSets[index];
            setInformation.setID = setDescription.m_SetID;
            // bindingID, offset
            std::map<uint32_t, size_t> bufferOffsets;
            for (auto& uniformDescription : setDescription.m_UniformBuffers)
            {
                DynamicUniformInformation uniformInfo;
                uniformInfo.bindingID = uniformDescription.m_Binding;
                uniformInfo.offset = totalSize;
                uniformInfo.bufferSize = vk::AlignSizeToUniformAlignment(context, uniformDescription.m_Size);
                uniformInfo.copyCount = 1;
                uniformInfo.usedCopies = 0;
                uniformInfo.startUsedCopies = 0;
                bufferOffsets.insert(std::make_pair(uniformDescription.m_Binding, totalSize));
                totalSize += uniformDescription.m_Size;
                setInformation.uniformInformation.insert(std::make_pair(uniformDescription.m_Binding, uniformInfo));
            }
            IBuffer2 buffer = Buffer2_Create(context, BufferType::UniformBuffer, totalSize, BufferMemoryType::STREAM);
            setInformation.buffer = buffer;
            reserved->setInformations.push_back(setInformation);
            VkWriteDescriptorSet writeInfo;
            writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfo.pNext = nullptr;
            writeInfo.dstSet = reserved->pDescriptorSets[index];
            writeInfo.dstArrayElement = 0;
            writeInfo.descriptorCount = 1;
            writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            writeInfo.pImageInfo = nullptr;
            writeInfo.pBufferInfo = nullptr;
            writeInfo.pTexelBufferView = nullptr;
            for (int b = 0; b < setDescription.m_UniformBuffers.size(); b++)
            {
                auto& uniformDescription = setDescription.m_UniformBuffers[b];

                VkDescriptorBufferInfo bufferInfo;
                bufferInfo.buffer = buffer->m_vk_buffer->m_buffer;
                bufferInfo.offset = bufferOffsets[uniformDescription.m_Binding];
                bufferInfo.range = uniformDescription.m_Size; // VK_WHOLE_SIZE;// uniformDescription.m_Size;

                writeInfo.dstBinding = uniformDescription.m_Binding;
                writeInfo.pBufferInfo = &bufferInfo;
                // Although we have bound a pipeline and started the renderpass
                // Its okay to call vkUpdateDescriptorSets since we have not actually stopped recording
                // and started executing the CommandList its like we have never started the vkUpdateDescriptorSets
                vkUpdateDescriptorSets(c->defaultDevice, 1, &writeInfo, 0, nullptr);
            }
            setInformation.isBuffer = true;
        }
        if (setDescription.m_SSBOs.size() > 0)
        {
            assert(programData->m_ssbos.size() > 0 && "You must set constant ssbos");
            DynamicSetInformation setInformation;
            setInformation.setHandleIndex = index;
            setInformation.setHandle = reserved->pDescriptorSets[index];
            setInformation.setID = setDescription.m_SetID;
            VkWriteDescriptorSet writeInfo;
            writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfo.pNext = nullptr;
            writeInfo.dstSet = reserved->pDescriptorSets[index];
            writeInfo.dstArrayElement = 0;
            writeInfo.descriptorCount = 1;
            writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeInfo.pImageInfo = nullptr;
            writeInfo.pBufferInfo = nullptr;
            writeInfo.pTexelBufferView = nullptr;
            for (int b = 0; b < setDescription.m_SSBOs.size(); b++)
            {
                VkDescriptorBufferInfo ssboInfo;
                ssboInfo.buffer = programData->m_ssbos[b].m_ssbobuffer->m_vk_buffer->m_buffer;
                ssboInfo.offset = 0;
                ssboInfo.range = VK_WHOLE_SIZE;

                writeInfo.dstBinding = programData->m_ssbos[b].m_bindingID;
                writeInfo.pBufferInfo = &ssboInfo;
                vkUpdateDescriptorSets(c->defaultDevice, 1, &writeInfo, 0, nullptr);
            }
            reserved->setInformations.push_back(setInformation);
        }
        if(setDescription.m_SampledImage.size() > 0)
        {
            DynamicSetInformation setInformation;
            setInformation.setHandleIndex = index;
            setInformation.setHandle = reserved->pDescriptorSets[index];
            setInformation.setID = setDescription.m_SetID;
            int viewIndex = 0;
            assert(programData->m_textures.size() > 0 && "You must provide your textures to IShaderProgramData");
            for (auto& sampledDescription : setDescription.m_SampledImage)
            {
                VkWriteDescriptorSet writeInfo;
                writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeInfo.pNext = nullptr;
                writeInfo.dstSet = reserved->pDescriptorSets[index];
                writeInfo.dstArrayElement = viewIndex;
                writeInfo.descriptorCount = 1;
                writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeInfo.pBufferInfo = nullptr;
                writeInfo.pTexelBufferView = nullptr;
                writeInfo.dstBinding = sampledDescription.m_Binding;

                VkDescriptorImageInfo imageInfo;
                imageInfo.sampler = (VkSampler)programData->m_sampler->m_NativeHandle;
                imageInfo.imageView = programData->m_textures[viewIndex]->view;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                writeInfo.pImageInfo = &imageInfo;
                vkUpdateDescriptorSets(c->defaultDevice, 1, &writeInfo, 0, nullptr);
                viewIndex++;
            }
            reserved->setInformations.push_back(setInformation);
        }
        index++;
    }
}

static void Vulkan_Internal_ExpandDescriptorSetBuffer(vk::VkContext context, ShaderProgramDataReserved* reserved, IPipelineLayout layout, DynamicSetInformation& setInformation, int bindingID, int AdditionalUseCount)
{
    PROFILE_FUNCTION();
    // TODO: Find out a way to update descriptor set buffer without
    // invalidating the command buffer and without calling vkDeviceWaitIdle
    // Calling vkDeviceWaitIdle is neccessary because without it, vkCmdBindDescriptorSets
    // causes a nullptr exception. I do not know if it is only in the validation layers.
    vkDeviceWaitIdle(context->defaultDevice);

    // You cannot update a descriptor set buffer while it is in use,
    // therefore we must also destroy the descriptor set
    // and allocate it again to be able to change it's buffer
    // vkFreeDescriptorSets(context->defaultDevice, reserved->descriptorPool, 1, &setInformation.setHandle);

    // VkDescriptorSetAllocateInfo allocateInfo;
    // allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    // allocateInfo.pNext = nullptr;
    // allocateInfo.descriptorPool = reserved->descriptorPool;
    // allocateInfo.descriptorSetCount = 1;
    // allocateInfo.pSetLayouts = &layout->m_real_setlayouts[setInformation.setHandleIndex];
    // vkcheck(vkAllocateDescriptorSets(context->defaultDevice, &allocateInfo, &setInformation.setHandle));

    // reserved->pDescriptorSets[setInformation.setHandleIndex] = setInformation.setHandle;

    IBuffer2 oldBuffer = setInformation.buffer;
    DynamicUniformInformation& uniformInfo = setInformation.uniformInformation[bindingID];
    uniformInfo.copyCount += AdditionalUseCount;
    int BlockSize = AdditionalUseCount * uniformInfo.bufferSize;
    setInformation.buffer = Buffer2_Create(context, oldBuffer->type, oldBuffer->size + BlockSize, oldBuffer->memoryType);
    Buffer2_Destroy(oldBuffer);

    // Change the buffer to every descriptor set binding

    VkDescriptorBufferInfo* pBufferInfos = (VkDescriptorBufferInfo*)stack_allocate(setInformation.uniformInformation.size() * sizeof(VkDescriptorBufferInfo));
    VkWriteDescriptorSet* pWriteInfos = (VkWriteDescriptorSet*)stack_allocate(setInformation.uniformInformation.size() * sizeof(VkWriteDescriptorSet));

    int offset = 0, index = 0;
    for (auto& _ui : setInformation.uniformInformation)
    {
        DynamicUniformInformation& ui = _ui.second;
        pBufferInfos[index].buffer = setInformation.buffer->m_vk_buffer->m_buffer;
        pBufferInfos[index].offset = offset;
        pBufferInfos[index].range = ui.bufferSize;
        ui.offset = offset;

        offset += ui.copyCount * ui.bufferSize;

        pWriteInfos[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pWriteInfos[index].pNext = nullptr;
        pWriteInfos[index].dstSet = setInformation.setHandle;
        pWriteInfos[index].dstArrayElement = 0;
        pWriteInfos[index].descriptorCount = 1;
        pWriteInfos[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pWriteInfos[index].pImageInfo = nullptr;
        pWriteInfos[index].pBufferInfo = nullptr;
        pWriteInfos[index].pTexelBufferView = nullptr;
        pWriteInfos[index].dstBinding = ui.bindingID;
        pWriteInfos[index].pBufferInfo = &pBufferInfos[index];
        index++;
    }
    // TODO: Fix invalid command buffer cause by update descriptor sets
    // The best solution to this is VK_KHR_push_descriptor but AMD does not support the extension
    // This does not cause any issues (only valdiations) and worst case just 3 frames.
    vkUpdateDescriptorSets(context->defaultDevice, setInformation.uniformInformation.size(), pWriteInfos, 0, nullptr);
    stack_free(pBufferInfos);
    stack_free(pWriteInfos);
}

static void Vulkan_Private_DestroyShaderProgramData(void* programData_reserved)
{
	PROFILE_FUNCTION();
	ShaderProgramDataReserved* reserved = (ShaderProgramDataReserved*)programData_reserved;
	vk::VkContext context = reserved->context;
	vkDestroyDescriptorPool(context->defaultDevice, reserved->descriptorPool, context->m_allocation_callback);
	delete[] reserved->pDescriptorSets;

	for (auto& setInfo : reserved->setInformations)
	{
		if (setInfo.isBuffer)
			Buffer2_Destroy(setInfo.buffer);
	}

	delete reserved;
}


void ShaderProgramData_Destroy(IShaderProgramData programData)
{
	if (programData->m_reserved)
	{
		Vulkan_Private_DestroyShaderProgramData(programData->m_reserved);
	}
	delete programData;
}