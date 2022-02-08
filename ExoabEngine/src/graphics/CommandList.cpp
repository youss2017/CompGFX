#include "CommandList.hpp"
#include "../core/backend/VkGraphicsCard.hpp"
#include "../core/backend/GLGraphicsCard.hpp"
#include "material_system/Material.hpp"
#include "CommandDefines.hpp"
#include <math.h>

PFN_CommandList_Create *CommandList_Create = nullptr;
PFN_CommandList_Destroy *CommandList_Destroy = nullptr;

PFN_CommandList_StartRecording *CommandList_StartRecording = nullptr;
PFN_CommandList_StopRecording *CommandList_StopRecording = nullptr;
PFN_CommandList_Reset *CommandList_Reset = nullptr;
PFN_CommandList_SetFramebuffer* CommandList_SetFramebuffer = nullptr;
PFN_CommandList_BindPipeline* CommandList_BindPipeline = nullptr;
PFN_CommandList_UpdateGPUBuffer *CommandList_UpdateGPUBuffer = nullptr;
PFN_CommandList_SetViewport *CommandList_SetViewport = nullptr;
PFN_CommandList_SetScissor *CommandList_SetScissor = nullptr;
PFN_CommandList_SetRenderState *CommandList_SetRenderState = nullptr;
PFN_CommandList_DrawArrays *CommandList_DrawArrays = nullptr;
PFN_CommandList_DrawIndexed *CommandList_DrawIndexed = nullptr;
PFN_CommandList_UpdateProgramData* CommandList_UpdateProgramData = nullptr;
PFN_CommandList_SetPushconstants* CommandList_SetPushconstants = nullptr;

// ============================================ VULKAN ============================================
ICommandList Vulkan_CommandList_Create(GraphicsContext context, CommandPool *pool);
void Vulkan_CommandList_Destroy(ICommandList command_list, CommandPool* pool, bool FreeFromCommandPool);

/* Commands */
void Vulkan_CommandList_StartRecording(ICommandList list, bool OneTimeSubmit, bool AllowUsageWhilePending);
void Vulkan_CommandList_StopRecording(ICommandList list);
void Vulkan_CommandList_Reset(ICommandList list, bool ReleaseCommandBufferResources);
void Vulkan_CommandList_SetFramebuffer(ICommandList list, IFramebufferStateManagement state_managment, IFramebuffer framebuffer);
void Vulkan_CommandList_BindPipeline(ICommandList list, IPipelineState pipeline_state);
void Vulkan_CommandList_UpdateGPUBuffer(ICommandList list, IGPUBuffer buffer, void *pData, uint32_t offset, uint32_t size);
void Vulkan_CommandList_SetViewport(ICommandList list, int x, int y, int width, int height);
void Vulkan_CommandList_SetScissor(ICommandList list, int x, int y, int width, int height);
void Vulkan_CommandList_SetRenderState(ICommandList list, RenderState *render_state, void* entity_for_instance_buffers);
void Vulkan_CommandList_DrawArrays(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t vertices_count, uint32_t instance_count);
void Vulkan_CommandList_DrawIndexed(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t indices_count, uint32_t instance_count);
void Vulkan_CommandList_SetPushconstants(ICommandList list, IPipelineState pipeline, const char* name, int offset, int size, void* pData);
/* Commands */
// ============================================ OPENGL ============================================
ICommandList GL_CommandList_Create(GraphicsContext context, CommandPool *pool);
void GL_CommandList_Destroy(ICommandList command_list, CommandPool* pool, bool FreeFromCommandPool);

/* Commands */
void GL_CommandList_StartRecording(ICommandList list, bool OneTimeSubmit, bool AllowUsageWhilePending);
void GL_CommandList_StopRecording(ICommandList list);
void GL_CommandList_Reset(ICommandList list, bool ReleaseCommandBufferResources);
void GL_CommandList_SetFramebuffer(ICommandList list, IFramebufferStateManagement state_managment, IFramebuffer framebuffer);
void GL_CommandList_BindPipeline(ICommandList list, IPipelineState pipeline_state);
void GL_CommandList_UpdateGPUBuffer(ICommandList list, IGPUBuffer buffer, void *pData, uint32_t offset, uint32_t size);
void GL_CommandList_SetViewport(ICommandList list, int x, int y, int width, int height);
void GL_CommandList_SetScissor(ICommandList list, int x, int y, int width, int height);
void GL_CommandList_SetRenderState(ICommandList list, RenderState *render_state, void* entity_for_instance_buffers);
void GL_CommandList_DrawArrays(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t vertices_count, uint32_t instance_count);
void GL_CommandList_DrawIndexed(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t indices_count, uint32_t instance_count);
void GL_CommandList_SetPushconstants(ICommandList list, IPipelineState pipeline, const char* name, int offset, int size, void* pData);
/* Commands */

void CommandList_LinkFunctions(GraphicsContext context)
{
    char ApiType = *(char *)context;
    if (ApiType == 0)
    {
        CommandList_Create = Vulkan_CommandList_Create;
        CommandList_Destroy = Vulkan_CommandList_Destroy;
        CommandList_StartRecording = Vulkan_CommandList_StartRecording;
        CommandList_StopRecording = Vulkan_CommandList_StopRecording;
        CommandList_Reset = Vulkan_CommandList_Reset;
        CommandList_SetFramebuffer = Vulkan_CommandList_SetFramebuffer;
        CommandList_BindPipeline = Vulkan_CommandList_BindPipeline;
        CommandList_UpdateGPUBuffer = Vulkan_CommandList_UpdateGPUBuffer;
        CommandList_SetViewport = Vulkan_CommandList_SetViewport;
        CommandList_SetScissor = Vulkan_CommandList_SetScissor;
        CommandList_SetRenderState = Vulkan_CommandList_SetRenderState;
        CommandList_DrawArrays = Vulkan_CommandList_DrawArrays;
        CommandList_DrawIndexed = Vulkan_CommandList_DrawIndexed;
        
        CommandList_SetPushconstants = Vulkan_CommandList_SetPushconstants;
    }
    else if (ApiType == 1)
    {
        CommandList_Create = GL_CommandList_Create;
        CommandList_Destroy = GL_CommandList_Destroy;
        CommandList_StartRecording = GL_CommandList_StartRecording;
        CommandList_StopRecording = GL_CommandList_StopRecording;
        CommandList_Reset = GL_CommandList_Reset;
        //CommandList_SetFramebuffer = GL_CommandList_SetFramebuffer;
        //CommandList_BindPipeline = GL_CommandList_BindPipeline;
        CommandList_UpdateGPUBuffer = GL_CommandList_UpdateGPUBuffer;
        CommandList_SetViewport = GL_CommandList_SetViewport;
        CommandList_SetScissor = GL_CommandList_SetScissor;
        CommandList_SetRenderState = GL_CommandList_SetRenderState;
        CommandList_DrawArrays = GL_CommandList_DrawArrays;
        CommandList_DrawIndexed = GL_CommandList_DrawIndexed;
        CommandList_SetPushconstants = GL_CommandList_SetPushconstants;
    }
    else
    {
        Utils::Break();
    }
}

