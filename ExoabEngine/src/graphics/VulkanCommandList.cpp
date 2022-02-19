#if 0
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
#endif