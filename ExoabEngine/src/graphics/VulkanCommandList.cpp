#include "CommandList.hpp"
#include "material_system/Material.hpp"
#include "../core/memory/Buffers.hpp"
#include "../utils/Profiling.hpp"
#include <map>

#if defined(_DEBUG)
#define LIST_GUARD() assert(list->m_CmdOpen && "Command list is not open!")
#define PROGRAM_GUARD() \
    LIST_GUARD();       \
    assert(list->m_FramebufferBound && "Framebuffer is not bound! This command must be called after framebuffer has been bound!")
#else
#define LIST_GUARD()
#define PROGRAM_GUARD()
#endif

#define VULKAN_COMMAND_LIST list->m_Cmd[ToVKContext(list->m_context)->FrameInfo->m_FrameIndex]
//#define VULKAN_COMMAND_LIST list->m_Cmd

ICommandList Vulkan_CommandList_Create(GraphicsContext context, CommandPool *pool)
{
    PROFILE_FUNCTION();
    auto vcont = ToVKContext(context);
    VkCommandBufferAllocateInfo AllocInfo;
    AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.pNext = nullptr;
    AllocInfo.commandPool = pool->GetPool();
    AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocInfo.commandBufferCount = vcont->FrameInfo->m_FrameCount;
    VkCommandBuffer *cmd = new VkCommandBuffer[vcont->FrameInfo->m_FrameCount];
    vkcheck(vkAllocateCommandBuffers(vcont->defaultDevice, &AllocInfo, cmd));
    CommandList *commandlist = new CommandList;
    commandlist->m_ApiType = 0;
    commandlist->m_Cmd = cmd;
    commandlist->m_context = context;
    commandlist->m_CmdOpen = false;
    commandlist->m_FramebufferBound = false;
    return commandlist;
}

void Vulkan_CommandList_Destroy(ICommandList command_list, CommandPool *pool, bool FreeFromCommandPool)
{
    if (FreeFromCommandPool)
    {
        auto vcont = ToVKContext(command_list->m_context);
        if (pool)
            vkFreeCommandBuffers(vcont->defaultDevice, pool->GetPool(), ToVKContext(command_list->m_context)->FrameInfo->m_FrameCount, command_list->m_Cmd);
        delete command_list->m_Cmd;
    }
    delete command_list;
}

/* Commands */
void Vulkan_CommandList_StartRecording(ICommandList list, bool OneTimeSubmit, bool AllowUsageWhilePending)
{
    assert(!list->m_CmdOpen);
    PROFILE_FUNCTION();
    VkCommandBufferBeginInfo BeginInfo;
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.pNext = nullptr;
    BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    BeginInfo.pInheritanceInfo = nullptr;
    vkcheck(vkBeginCommandBuffer(VULKAN_COMMAND_LIST, &BeginInfo));
    list->m_CmdOpen = true;
}

void Vulkan_CommandList_StopRecording(ICommandList list)
{
    PROFILE_FUNCTION();
    LIST_GUARD();
    if (list->m_FramebufferBound)
    {
        vkCmdEndRenderPass(VULKAN_COMMAND_LIST);
        list->m_FramebufferBound = false;
    }
    vkEndCommandBuffer(VULKAN_COMMAND_LIST);
    list->m_CmdOpen = false;
}

void Vulkan_CommandList_Reset(ICommandList list, bool ReleaseCommandBufferResources)
{
    assert(!list->m_CmdOpen);
    PROFILE_FUNCTION();
    vkcheck(vkResetCommandBuffer(VULKAN_COMMAND_LIST, ReleaseCommandBufferResources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0));
    list->m_program_data_counter = 0;
}

void Vulkan_CommandList_SetFramebuffer(ICommandList list, IFramebufferStateManagement state_managment, IFramebuffer framebuffer)
{
    PROFILE_FUNCTION();
    LIST_GUARD();
    if (list->m_FramebufferBound)
    {
        vkCmdEndRenderPass(VULKAN_COMMAND_LIST);
    }
    uint32_t ClearValueCount = 0;
    auto& attachments = state_managment->m_attachments;
    VkClearValue* pClearValues;
    {
        PROFILE_SCOPE("[Vulkan] Preparing Clear Values.");
        pClearValues = (VkClearValue*)stack_allocate(sizeof(VkClearValue) * (attachments.size() + 1));
        for (size_t i = 0, j = 0; i < attachments.size(); i++)
        {
            auto& temp = attachments[i];
            if (temp.m_loading == VK_ATTACHMENT_LOAD_OP_CLEAR)
            {
                pClearValues[j++] = temp.clearColor;
                ClearValueCount++;
            }
            else
            {
                // Even if the
                pClearValues[j++] = { 0.0, 0.0, 0.0, 0.0 };
                ClearValueCount++;
            }
        }
        if (state_managment->m_depth_attachment.has_value())
        {
            auto depth_attachment = state_managment->m_depth_attachment.value();
            if (depth_attachment.m_loading == VK_ATTACHMENT_LOAD_OP_CLEAR)
                pClearValues[ClearValueCount++] = depth_attachment.clearColor;
        }
    }
    VkRenderPassBeginInfo BeginInfo;
    VkViewport viewport;
    VkRect2D scissor;
    BeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    BeginInfo.pNext = nullptr;
    BeginInfo.renderPass = state_managment->m_renderpass;
    BeginInfo.framebuffer = (VkFramebuffer)Framebuffer_Get(framebuffer);
    framebuffer->GetViewport(BeginInfo.renderArea.offset.x, BeginInfo.renderArea.offset.y, BeginInfo.renderArea.extent.width, BeginInfo.renderArea.extent.height);
    framebuffer->GetViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    framebuffer->GetScissor(scissor.offset.x, scissor.offset.y, scissor.extent.width, scissor.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    BeginInfo.clearValueCount = ClearValueCount;
    BeginInfo.pClearValues = pClearValues;
    {
        PROFILE_SCOPE("[Vulkan] Recording Commands.");
        vkCmdBeginRenderPass(VULKAN_COMMAND_LIST, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(VULKAN_COMMAND_LIST, 0, 1, &viewport);
        vkCmdSetScissor(VULKAN_COMMAND_LIST, 0, 1, &scissor);
    }
    list->m_FramebufferBound = true;
    stack_free(pClearValues);
}

void Vulkan_CommandList_BindPipeline(ICommandList list, IPipelineState pipeline_state)
{
    LIST_GUARD();
    PROGRAM_GUARD();
    vkCmdBindPipeline(VULKAN_COMMAND_LIST, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pipeline_state->m_pipeline);
}

void Vulkan_CommandList_UpdateGPUBuffer(ICommandList list, IBuffer2 buffer, void *pData, uint32_t offset, uint32_t size)
{
    PROFILE_FUNCTION();
    LIST_GUARD();
    assert(size < pow(2, 16) && !list->m_FramebufferBound && "Cannot not UpdateGPUBuffer inside program and the size of the update cannot be bigger than 2^16 bytes.");
    vkCmdUpdateBuffer(VULKAN_COMMAND_LIST, buffer->m_vk_buffer->m_buffer, offset, size, pData);
}

void Vulkan_CommandList_SetViewport(ICommandList list, int x, int y, int width, int height)
{
    PROFILE_FUNCTION();
    PROGRAM_GUARD();
    VkViewport viewport;
    viewport.x = 0, viewport.y = 0, viewport.width = width, viewport.height = height, viewport.minDepth = 0.0f, viewport.maxDepth = 1.0f;
    vkCmdSetViewport(VULKAN_COMMAND_LIST, 0, 1, &viewport);
}

void Vulkan_CommandList_SetScissor(ICommandList list, int x, int y, int width, int height)
{
    PROFILE_FUNCTION();
    PROGRAM_GUARD();
    VkRect2D scissor;
    scissor.offset = {x, y};
    scissor.extent = {(uint32_t)width, (uint32_t)height};
    vkCmdSetScissor(VULKAN_COMMAND_LIST, 0, 1, &scissor);
}

void Vulkan_CommandList_SetRenderState(ICommandList list, RenderState *render_state, void* entity_for_instance_buffers)
{
    PROFILE_FUNCTION();
    PROGRAM_GUARD();
    if (render_state->m_UsingIndexBuffer)
        vkCmdBindIndexBuffer(VULKAN_COMMAND_LIST, (VkBuffer)render_state->m_IndexBuffer->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
    VkDeviceSize pOffset[1] = {0};
    //vkCmdBindVertexBuffers(VULKAN_COMMAND_LIST, 0, 1, &render_state->m_VertexBuffer->m_vk_buffer->m_buffer, pOffset);
    if (entity_for_instance_buffers)
    {
        DefaultEntity* entity = (DefaultEntity*)entity_for_instance_buffers;
#if defined(_DEBUG)
        if (entity->VkVertexInstanceBuffers.size() == 0)
        {
            log_warning("Attempted to set instance buffer but entity does not have any instance buffers bound.", false);
            return;
        }
#endif
        vkCmdBindVertexBuffers(VULKAN_COMMAND_LIST, 1, entity->VkVertexInstanceBuffers.size(), entity->VkVertexInstanceBuffers.data(), entity->VertexInstanceBufferOffset.data());
    }
}

void Vulkan_CommandList_SetPushconstants(ICommandList list, IPipelineState pipeline, const char *name, int offset, int size, void *pData)
{
    PROGRAM_GUARD();
    vkCmdPushConstants(VULKAN_COMMAND_LIST, (VkPipelineLayout)pipeline->m_layout->m_pipelinelayout, VK_SHADER_STAGE_FRAGMENT_BIT, offset, size, pData);
}

#include "material_system/ShaderProgramData_Internal.inl"

void Vulkan_CommandList_DrawArrays(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t vertices_count, uint32_t instance_count)
{
    PROGRAM_GUARD();
    auto reserved = (ShaderProgramDataReserved*)program_data->m_reserved;
    VkDescriptorSet* pDescriptorSets = (VkDescriptorSet*)alloca(sizeof(VkDescriptorSet) * reserved->descriptorSetCount);
    uint32_t firstSet = 0;
    uint32_t descriptorCount = reserved->descriptorSetCount;
    std::vector<uint32_t> dynamicOffsets;
    {
        PROFILE_SCOPE("Calculating Offsets");
        // To avoid multiple memory heap allocations
        dynamicOffsets.reserve(reserved->descriptorSetCount * 3);
        // When entityCount is greater than 1, startUsedCopies is not restored to zero idk
        // This bugoffset is used to make sure the dynamic offsets are correct.
        int BugOffset = update_index > 1 ? -1 : 0;
        for (uint32_t i = 0; i < reserved->descriptorSetCount; i++)
        {
            auto& setInfo = reserved->setInformations[i];
            if (setInfo.isBuffer)
            {
                pDescriptorSets[i] = reserved->pDescriptorSets[i];
                uint32_t dynamicOffsetCount = setInfo.uniformInformation.size();
                for (uint32_t q = 0; q < dynamicOffsetCount; q++)
                {
                    auto& uniformInfo = setInfo.uniformInformation[q];
                    dynamicOffsets.push_back(uniformInfo.bufferSize * (update_index + uniformInfo.startUsedCopies + BugOffset));
                }
            }
            else
            {
                pDescriptorSets[i] = setInfo.setHandle;
            }
        }
    }
    VkPipelineLayout pipelineLayout = (VkPipelineLayout)layout->m_pipelinelayout;
    vkCmdBindDescriptorSets(VULKAN_COMMAND_LIST, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, firstSet, descriptorCount, pDescriptorSets, dynamicOffsets.size(), dynamicOffsets.data());
    vkCmdDraw(VULKAN_COMMAND_LIST, vertices_count, instance_count, vertices_offset, 0);
}

void Vulkan_CommandList_DrawIndexed(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t indices_count, uint32_t instance_count)
{
    PROGRAM_GUARD();
    auto reserved = (ShaderProgramDataReserved*)program_data->m_reserved;
    VkDescriptorSet* pDescriptorSets = (VkDescriptorSet*)alloca(sizeof(VkDescriptorSet) * reserved->descriptorSetCount);
    uint32_t firstSet = 0;
    uint32_t descriptorCount = reserved->descriptorSetCount;
    std::vector<uint32_t> dynamicOffsets;
    {
        PROFILE_SCOPE("Calculating Offsets");
        // To avoid multiple memory heap allocations
        dynamicOffsets.reserve(reserved->descriptorSetCount * 3);
        // When entityCount is greater than 1, startUsedCopies is not restored to zero idk
        // This bugoffset is used to make sure the dynamic offsets are correct.
        int BugOffset = update_index > 1 ? -1 : 0;
        for (uint32_t i = 0; i < reserved->descriptorSetCount; i++)
        {
            auto& setInfo = reserved->setInformations[i];
            if (!setInfo.isBuffer)
            {
                pDescriptorSets[i] = setInfo.setHandle;
                continue;
            }
            pDescriptorSets[i] = reserved->pDescriptorSets[i];
            uint32_t dynamicOffsetCount = setInfo.uniformInformation.size();
            for (uint32_t q = 0; q < dynamicOffsetCount; q++)
            {
                auto& uniformInfo = setInfo.uniformInformation[q];
                dynamicOffsets.push_back(uniformInfo.bufferSize * (update_index + uniformInfo.startUsedCopies + BugOffset));
            }
        }
    }
    VkPipelineLayout pipelineLayout = (VkPipelineLayout)layout->m_pipelinelayout;
    vkCmdBindDescriptorSets(VULKAN_COMMAND_LIST, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, firstSet, descriptorCount, pDescriptorSets, dynamicOffsets.size(), dynamicOffsets.data());
    vkCmdDrawIndexed(VULKAN_COMMAND_LIST, indices_count, instance_count, 0, vertices_offset, 0);
}

/* Commands */
