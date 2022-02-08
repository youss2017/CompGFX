#pragma once
#include "../core/backend/backend_base.h"
#include "../core/pipeline/Pipeline.hpp"
#include "../units/Entity.hpp"
#include "material_system/ShaderProgramData.hpp"
#include "RenderState.hpp"
#include <vulkan/vulkan.h>
#include <vector>

struct Graphics3D;
typedef Graphics3D *IGraphics3D;
class GPUFence;
class GPUSemaphore;

// TODO: Optimize the size of GLCommandData and heap allocations
struct GLCommandData
{
    // Command ID
    int m_CmdID;
    // Command Data

    union {
        uint32_t vertices_offset;
        uint32_t offset;
    };
    union {
        uint32_t vertices_count;
        uint32_t size;
    };
    uint32_t instance_count;
    uint32_t indices_count;
    uint32_t x, y, width, height; // viewport, scissor
    RenderState render_state;     // vertex buffer, index buffer
    IPipelineState pipeline_state;
    IFramebuffer fbo;              // start pass, fbo
    union {
        void* material;
        void* pData;
    };

    const char* name;
    IShaderProgramData program_data;

    std::vector<DefaultEntity> entities;
};

// TODO: Move this to function pointer style.
class CommandPool
{
public:
    static CommandPool Create(GraphicsContext context, bool ShortLivedListBuffers, bool AllowIndividualResets);
    void Reset(bool ReleasePoolResources);
    void Destroy();
    const inline VkCommandPool GetPool() { return m_pool; }

private:
    int m_ApiType;
    VkCommandPool m_pool;
    GraphicsContext m_context;
};

// TODO: Support secondary command buffers
struct CommandList
{
    int m_ApiType;
    VkCommandBuffer* m_Cmd;
    std::vector<GLCommandData> m_GLCmd;
    GraphicsContext m_context;
    uint32_t m_program_data_counter = 0;
    bool m_CmdOpen, m_FramebufferBound;
};

#define VkCommandList_Get(command_list) command_list->m_Cmd[ToVKContext(command_list->m_context)->FrameInfo->m_FrameIndex]

typedef CommandList *ICommandList;

typedef ICommandList PFN_CommandList_Create(GraphicsContext context, CommandPool *pool);
typedef void PFN_CommandList_Destroy(ICommandList command_list, CommandPool* pool, bool FreeFromCommandPool);

/* Commands */
typedef void PFN_CommandList_StartRecording(ICommandList list, bool OneTimeSubmit, bool AllowUsageWhilePending);
typedef void PFN_CommandList_StopRecording(ICommandList list);
typedef void PFN_CommandList_Reset(ICommandList list, bool ReleaseCommandBufferResources);
typedef void PFN_CommandList_SetFramebuffer(ICommandList list, IFramebufferStateManagement state_managment, IFramebuffer framebuffer);
typedef void PFN_CommandList_BindPipeline(ICommandList list, IPipelineState pipeline_state);
typedef void PFN_CommandList_UpdateGPUBuffer(ICommandList list, IGPUBuffer buffer, void *pData, uint32_t offset, uint32_t size);
typedef void PFN_CommandList_SetViewport(ICommandList list, int x, int y, int width, int height);
typedef void PFN_CommandList_SetScissor(ICommandList list, int x, int y, int width, int height);
typedef void PFN_CommandList_SetRenderState(ICommandList list, RenderState *render_state, void* entity_for_instance_buffers);
typedef void PFN_CommandList_DrawArrays(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t vertices_count, uint32_t instance_count);
typedef void PFN_CommandList_DrawIndexed(ICommandList list, IPipelineLayout layout, IShaderProgramData program_data, uint32_t update_index, uint32_t vertices_offset, uint32_t indices_count, uint32_t instance_count);
typedef void PFN_CommandList_UpdateProgramData(ICommandList list, IShaderProgramData layout, int setID, int bindingID, const char* UniformName, void* pData, uint32_t size);

// TODO: Combine pushconstants with shader program data.
typedef void PFN_CommandList_SetPushconstants(ICommandList list, IPipelineState pipeline, const char* name, int offset, int size, void* pData);

/* Commands */

extern PFN_CommandList_Create* CommandList_Create;
extern PFN_CommandList_Destroy* CommandList_Destroy;

extern PFN_CommandList_StartRecording* CommandList_StartRecording;
extern PFN_CommandList_StopRecording* CommandList_StopRecording;
extern PFN_CommandList_Reset* CommandList_Reset;
extern PFN_CommandList_SetFramebuffer* CommandList_SetFramebuffer;
extern PFN_CommandList_BindPipeline* CommandList_BindPipeline;
extern PFN_CommandList_UpdateGPUBuffer* CommandList_UpdateGPUBuffer;
extern PFN_CommandList_SetViewport* CommandList_SetViewport;
extern PFN_CommandList_SetScissor* CommandList_SetScissor;
extern PFN_CommandList_SetRenderState* CommandList_SetRenderState;
extern PFN_CommandList_DrawArrays* CommandList_DrawArrays;
extern PFN_CommandList_DrawIndexed* CommandList_DrawIndexed;
extern PFN_CommandList_UpdateProgramData* CommandList_UpdateProgramData;
extern PFN_CommandList_SetPushconstants* CommandList_SetPushconstants;

void CommandList_LinkFunctions(GraphicsContext context);
