#include "CommandList.hpp"
#include "CommandDefines.hpp"
#include "material_system/Material.hpp"
#include "../utils/StringUtils.hpp"

#define LIST_GUARD() assert(list->m_CmdOpen && "Command list is not open!")
#define PROGRAM_GUARD() \
    LIST_GUARD();       \
    assert(list->m_FramebufferBound && "Shader Program is has not started! This command must be called inside shader program!.")

ICommandList GL_CommandList_Create(GraphicsContext context, CommandPool *pool)
{
    CommandList *commandlist = new CommandList();
    commandlist->m_ApiType = 1;
    commandlist->m_Cmd = nullptr;
    commandlist->m_context = context;
    commandlist->m_CmdOpen = false;
    commandlist->m_FramebufferBound = false;
    return commandlist;
}

void GL_CommandList_Destroy(ICommandList command_list, CommandPool* pool, bool FreeFromCommandPool)
{
    delete command_list;
}

/* Commands */
void GL_CommandList_StartRecording(ICommandList list, bool OneTimeSubmit, bool AllowUsageWhilePending)
{
    list->m_GLCmd.clear();
    list->m_CmdOpen = true;
}

void GL_CommandList_StopRecording(ICommandList list)
{
    list->m_CmdOpen = false;
}

void GL_CommandList_Reset(ICommandList list, bool ReleaseCommandBufferResources)
{
    if (ReleaseCommandBufferResources)
        list->m_GLCmd.clear();
    else
        list->m_GLCmd.clear();
}

void GL_CommandList_SwitchMaterial(ICommandList list, void *_material)
{
    LIST_GUARD();
    assert(!list->m_FramebufferBound && "Cannot start Shader Program and use a material inside it!");
    list->m_FramebufferBound = true;
    Material *material = (Material *)_material;
    GLCommandData command;
    command.m_CmdID = COMMAND_SWITCH_MATERIAL;
    command.material = material;
    list->m_GLCmd.push_back(command);
}

void GL_CommandList_EndMaterial(ICommandList list)
{
    LIST_GUARD();
    assert(list->m_FramebufferBound && "Cannot stop Shader Program that has not started!");
    list->m_FramebufferBound = false;
    GLCommandData command;
    command.m_CmdID = COMMAND_STOP_SHADER_PROGRAM;
    list->m_GLCmd.push_back(command);
}

void GL_CommandList_UpdateGPUBuffer(ICommandList list, IGPUBuffer buffer, void *pData, uint32_t offset, uint32_t size)
{
    LIST_GUARD();
    assert(size < pow(2, 16) && !list->m_FramebufferBound && "Cannot not UpdateGPUBuffer inside program and the size of the update cannot be bigger than 2^16 bytes.");
    GPUBuffer_UploadData(buffer, pData, offset, size);
}
void GL_CommandList_SetViewport(ICommandList list, int x, int y, int width, int height)
{
    PROGRAM_GUARD();
    GLCommandData command;
    command.m_CmdID = COMMAND_SET_VIEWPORT;
    command.x = x, command.y = y, command.width = width, command.height = height;
    list->m_GLCmd.push_back(command);
}
void GL_CommandList_SetScissor(ICommandList list, int x, int y, int width, int height)
{
    PROGRAM_GUARD();
    GLCommandData command;
    command.m_CmdID = COMMAND_SET_SCISSOR;
    command.x = x, command.y = y, command.width = width, command.height = height;
    list->m_GLCmd.push_back(command);
}

void GL_CommandList_SetRenderState(ICommandList list, RenderState *render_state, void* entity_for_instance_buffers)
{
    PROGRAM_GUARD();
    GLCommandData command;
    command.m_CmdID = COMMAND_SET_RENDERSTATE;
    command.render_state = *render_state;
    list->m_GLCmd.push_back(command);
}

void GL_CommandList_DrawArrays(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t vertices_count, uint32_t instance_count)
{
    PROGRAM_GUARD();
    GLCommandData command;
    command.m_CmdID = COMMAND_DRAW_ARRAYS;
    command.vertices_offset = vertices_offset;
    command.vertices_count = vertices_count;
    command.instance_count = instance_count;
    list->m_GLCmd.push_back(command);
}

void GL_CommandList_DrawIndexed(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t indices_count, uint32_t instance_count)
{
    PROGRAM_GUARD();
    GLCommandData command;
    command.m_CmdID = COMMAND_DRAW_INDEXED;
    command.vertices_offset = vertices_offset;
    command.indices_count = indices_count;
    command.instance_count = instance_count;
    list->m_GLCmd.push_back(command);
}

void GL_CommandList_SetPushconstants(ICommandList list, IPipelineState pipeline, const char* name, int offset, int size, void* pData)
{
    PROGRAM_GUARD();
    GLCommandData command;
    command.pipeline_state = pipeline;
    command.name = name;
    command.offset = offset;
    command.size = size;
    list->m_GLCmd.push_back(command);
}

/* Commands */
