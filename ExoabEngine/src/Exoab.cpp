#include "Exoab.hpp"
#include "Camera.hpp"
#include "units/Map.hpp"
#include "ShaderTypes.hpp"
#include "UI.hpp"
#include <memory/Buffer2.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <backend/VkGraphicsCard.hpp>
#include "mesh/geometry.hpp"

/*
    *** NOTE *** The file is only used for testing.
*/

constexpr bool Graphics_EnableDebug = true;

static ConfigurationSettings s_Config;
static IGraphics3D s_Gfx;
static PlatformWindow *s_Window;
static vk::VkContext s_Context;
static IGPUTexture2D s_Tex0;
static IGPUTextureSampler s_Sampler0;
static Material s_Material0;
static ImFont* s_Font;
static IBuffer2 s_ObjectDataBuffer;
static IBuffer2 s_DrawDataBuffer;
static ShaderTypes::ObjectData* s_ObjectDataPtr;
static ShaderTypes::DrawData* s_DrawDataPtr;
constexpr uint32_t ss_MaxObjects = 10000;
static VkCommandPool s_Pool0;
static VkCommandBuffer s_Cmd[3];
static IBuffer2 s_VerticesSSBO;
static IBuffer2 s_IndicesBuffer;
static std::vector<Mesh::Geometry> s_Geomtry;
static VkSemaphore s_Semaphores[3];
static vk::GraphicsSwapchain s_Swapchain;

FramebufferReserve* g_FramebufferReserve = nullptr;

/*
    TODO:
    [Maybe OpenGL implementation]
    MSAA Support
    Map
    Animation
    PhysX
    Audio
    Quick ImGui Menu
    VK_EXT_memory_budget
*/

static constexpr const char* ss_ShaderCache = "assets/materials/shaders/spirv_cache/";
static constexpr const char* ss_FramebufferReserve = "assets/materials/framebuffer_reserve.cfg";
#ifdef _DEBUG
static constexpr bool ss_DebugMode = true;
#else
static constexpr bool ss_DebugMode = false;
#endif

bool Exoab_Initalize(ConfigurationSettings config)
{
    PROFILE_FUNCTION();
    //log_configure(0, false, false);
    {
        PROFILE_SCOPE("Creating Graphics3D");
        s_Config = config;
        s_Config.FutureFrameCount = 12;
        s_Config.VSync = false;
        s_Gfx = Graphics3D_Create(&s_Config, s_Config.WindowTitle.c_str(), ss_DebugMode, true);
        s_Window = s_Gfx->m_window;
        s_Context = ToVKContext(s_Gfx->m_context);
        s_Swapchain = s_Gfx->m_vswapchain;
    }

    // Configure Shader Cache
    Shader::ConfigureShaderCache(ss_ShaderCache);
    g_FramebufferReserve = new FramebufferReserve(s_Context, config, ss_FramebufferReserve);

    assert(Mesh::LoadVerticesIndicesSSBOs(s_Context, { "assets/mesh/cube.obj" }, s_Geomtry, &s_VerticesSSBO, &s_IndicesBuffer));

    MaterialConfiguration uniform_material_mc = MaterialConfiguration("assets/materials/uniforms.mc");

    PipelineVertexInputDescription input_description;
    IPipelineShaders shaders = Material_CreatePipelineShaders(s_Context, &uniform_material_mc);
    IMaterialPipelineLayout layout = Material_CreatePipelineLayout(s_Context, input_description, shaders);
    IFramebufferStateManagement state_managment = Material_CreateFramebufferStateManagment(s_Context, &uniform_material_mc, g_FramebufferReserve);
    IMaterialFramebuffer framebuffer = Material_CreateFramebuffer(s_Context, &uniform_material_mc, config, state_managment, g_FramebufferReserve);
    IPipelineState pipeline = Material_CreatePipelineState(s_Context, &uniform_material_mc, layout, state_managment);

    s_Material0.Initalize(shaders, layout, state_managment, framebuffer, pipeline);
    s_ObjectDataBuffer = Buffer2_Create(s_Context, BufferType::StorageBuffer, sizeof(ShaderTypes::ObjectData) * ss_MaxObjects, BufferMemoryType::CPU_TO_CPU);
    s_DrawDataBuffer = Buffer2_Create(s_Context, BufferType(StorageBuffer | IndirectBuffer), sizeof(ShaderTypes::DrawData) * ss_MaxObjects, BufferMemoryType::CPU_TO_CPU);
    s_ObjectDataPtr = (ShaderTypes::ObjectData*)Buffer2_Map(s_ObjectDataBuffer);
    s_DrawDataPtr = (ShaderTypes::DrawData*)Buffer2_Map(s_DrawDataBuffer);

    s_Pool0 = vk::Gfx_CreateCommandPool(s_Context, true, false, false);
    s_Cmd[0] = vk::Gfx_AllocCommandBuffer(s_Context, s_Pool0, true);
    s_Cmd[1] = vk::Gfx_AllocCommandBuffer(s_Context, s_Pool0, true);
    s_Cmd[2] = vk::Gfx_AllocCommandBuffer(s_Context, s_Pool0, true);

    s_Tex0 = GPUTexture2D_CreateFromFile(s_Context, "assets/textures/statue.jpg", true);
    s_Sampler0 = GPUTextureSampler_CreateDefaultSampler(s_Context);

    for (int i = 0; i < s_Geomtry.size(); i++) {
        s_DrawDataPtr[i].command.indexCount = s_Geomtry[i].indicesCount;
        s_DrawDataPtr[i].command.instanceCount = 1;
        s_DrawDataPtr[i].command.firstIndex = s_Geomtry[i].firstIndex;
        s_DrawDataPtr[i].command.vertexOffset = s_Geomtry[i].firstVertex;
        s_DrawDataPtr[i].command.firstInstance = 0;
        s_DrawDataPtr[i].ObjectDataIndex = 0;
    }
  
    s_Semaphores[0] = vk::Gfx_CreateSemaphore(s_Context, false);
    s_Semaphores[1] = vk::Gfx_CreateSemaphore(s_Context, false);
    s_Semaphores[2] = vk::Gfx_CreateSemaphore(s_Context, false);
    
    UI::Initalize(s_Context, s_Gfx);

    return true;
}

static Camera s_Camera({0, 0, 0});

void BuildCommandBuffers(double dTimeStart);

void Exoab_Render(double dTimeStart, double dElapsedTime, double FrameRate)
{
    PROFILE_FUNCTION();
    if (s_Window->GetWidth() == 0 || s_Window->GetHeight() == 0)
        return; // window is minimized cannot create a swapchain size 0,0
    VkSemaphore SwapchainReady;
    s_Swapchain.PrepareNextFrame(s_Context->pFrameIndex, &SwapchainReady);
    
    glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(0.0, 0.0, -5.0)) * glm::rotate(glm::mat4(1.0), (float)dTimeStart, glm::vec3(0., 1.0f, 0.));
    s_ObjectDataPtr[0].m_Model = model;
    s_ObjectDataPtr[0].m_NormalModel = glm::transpose(glm::inverse(model));
    s_ObjectDataPtr[0].m_View = s_Camera.GetViewMatrix();
    s_ObjectDataPtr[0].m_Projection = (glm::perspective(glm::radians(90.0f), (float)s_Window->m_width / (float)s_Window->m_height, 0.1f, 2000.f));
    Buffer2_Flush(s_ObjectDataBuffer, 0, sizeof(ShaderTypes::ObjectData));

    BuildCommandBuffers(0);

    //Graphics3D_ExecuteCommandList(s_Gfx, s_Cmd, nullptr, 1, &SwapchainReady, 1, &s_Semaphore);
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &SwapchainReady;
    submitInfo.pWaitDstStageMask = flags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &s_Cmd[*s_Context->pFrameIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &s_Semaphores[*s_Context->pFrameIndex];

    UI::FrameRate = FrameRate;
    UI::RenderUI();
    //Graphics3D_Present(s_Gfx, s_Material0.m_framebuffer->m_framebuffer, UI::ShowDepthBuffer ? 1 : 0, 1, &s_Semaphore, UI::ShowDepthBuffer);
    auto i = *s_Context->pFrameIndex;
    s_Swapchain.Present(s_Material0.m_framebuffer->m_textures[0]->m_images[i], s_Material0.m_framebuffer->m_textures[0]->m_views[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, &s_Semaphores[i], false);
    Graphics3D_WaitGPUIdle(s_Gfx);
}

void BuildCommandBuffers(double dTimeStart)
{
    PROFILE_FUNCTION();

    //CommandList_Reset(s_Cmd, false);
    VkCommandBuffer cmd = s_Cmd[*s_Context->pFrameIndex];
    vkResetCommandBuffer(cmd, 0);
    vk::Gfx_StartCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    //CommandList_StartRecording(s_Cmd, true, false);

    //CommandList_SetFramebuffer(s_Cmd, s_Material0.m_state_managment, s_Material0.m_framebuffer->m_framebuffer);
    
    uint32_t ClearValueCount = 0;
    auto& attachments = s_Material0.m_state_managment->m_attachments;
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
        if (s_Material0.m_state_managment->m_depth_attachment.has_value())
        {
            auto depth_attachment = s_Material0.m_state_managment->m_depth_attachment.value();
            if (depth_attachment.m_loading == VK_ATTACHMENT_LOAD_OP_CLEAR)
                pClearValues[ClearValueCount++] = depth_attachment.clearColor;
        }
    }
    VkRenderPassBeginInfo BeginInfo;
    VkViewport viewport;
    VkRect2D scissor;
    BeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    BeginInfo.pNext = nullptr;
    BeginInfo.renderPass = s_Material0.m_state_managment->m_renderpass;
    BeginInfo.framebuffer = (VkFramebuffer)Framebuffer_Get(s_Material0.m_framebuffer->m_framebuffer);
    s_Material0.m_framebuffer->m_framebuffer->GetViewport(BeginInfo.renderArea.offset.x, BeginInfo.renderArea.offset.y, BeginInfo.renderArea.extent.width, BeginInfo.renderArea.extent.height);
    s_Material0.m_framebuffer->m_framebuffer->GetViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    s_Material0.m_framebuffer->m_framebuffer->GetScissor(scissor.offset.x, scissor.offset.y, scissor.extent.width, scissor.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    BeginInfo.clearValueCount = ClearValueCount;
    BeginInfo.pClearValues = pClearValues;
    {
        PROFILE_SCOPE("[Vulkan] Recording Commands.");
        vkCmdBeginRenderPass(cmd, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }
    
    //CommandList_BindPipeline(s_Cmd, s_Material0.m_pipeline_state);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)s_Material0.m_pipeline_state->m_pipeline);
    s_ObjectDataPtr[0].m_Projection = (glm::perspectiveRH(glm::radians(45.0f), (float)s_Window->m_width / (float)s_Window->m_height, 0.1f, 2000.f));
    
    // bind descriptor set here
    vkCmdBindIndexBuffer(cmd, s_IndicesBuffer->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT16);
    // todo: switch to count.
    vkCmdDrawIndexedIndirect(cmd, s_DrawDataBuffer->m_vk_buffer->m_buffer, 0, 1, sizeof(ShaderTypes::DrawData));

    glm::vec3 LightDirection = glm::vec3(0, 1, 0);

    vkEndCommandBuffer(cmd);
}

Tristate Exoab_Update(double dTimeStart, double dElapsedTime)
{
    PROFILE_FUNCTION();
    if (UI::StateChanged)
    {
        UI::StateChanged = false;
        CheckActionTime(Graphics3D_WaitGPUIdle(s_Gfx));
        auto spec = s_Material0.m_pipeline_state->m_spec;
        spec.m_PolygonMode = UI::ShowWireframe ? PolygonMode::LINE : PolygonMode::FILL;
        PipelineState_Destroy(s_Material0.m_pipeline_state);
        s_Material0.m_pipeline_state = PipelineState_Create(s_Context, spec, s_Material0.m_state_managment, s_Material0.m_pipeline_layout->m_layout, s_Material0.m_pipeline_shaders->m_vertex_shader, s_Material0.m_pipeline_shaders->m_fragment_shader);
    }
    if (!s_Window->IsWindowFocus())
    {
        Graphics3D_PollEvents(s_Gfx);
        return Tristate::Enabled();
    }
    /* Camera Controls */
    double RotateRate = 45.0;
    if (s_Window->IsKeyDown('w')) {
        s_Camera.MoveForward(dElapsedTime * -22.0);
    }if (s_Window->IsKeyDown('s')) {
        s_Camera.MoveForward(dElapsedTime * 22.0);
    }if (s_Window->IsKeyDown('a')) {
        s_Camera.MoveSideways(dElapsedTime * -22.0);
    }if (s_Window->IsKeyDown('d')) {
        s_Camera.MoveSideways(dElapsedTime * 22.0);
    }if (s_Window->IsKeyDown('q')) {
        s_Camera.Yaw(dElapsedTime * RotateRate, true);
    }if (s_Window->IsKeyDown('e')) {
        s_Camera.Yaw(dElapsedTime * -RotateRate, true);
    }if (s_Window->IsKeyDown('z')) {
        s_Camera.Pitch(dElapsedTime * RotateRate, true);
    }if (s_Window->IsKeyDown('x')) {
        s_Camera.Pitch(dElapsedTime * -RotateRate, true);
    }if (s_Window->IsKeyDown(GLFW_KEY_UP)) {
        s_Camera.MoveAlongUpAxis(dElapsedTime * -22.0);
    }if (s_Window->IsKeyDown(GLFW_KEY_DOWN)) {
        s_Camera.MoveAlongUpAxis(dElapsedTime * 22.0);
    }
    auto position = s_Camera.GetPosition();
    UI::C_x = position.x, UI::C_y = position.y, UI::C_z = position.z;
    if (s_Window->IsKeyUp(GLFW_KEY_ESCAPE))
        return Tristate::Disabled();
    return Graphics3D_PollEvents(s_Gfx) == true ? Tristate::Enabled() : Tristate::Disabled();
}

void Exoab_CleanUp()
{
    PROFILE_FUNCTION();
    loginfo("Goodbye!");
    Graphics3D_WaitGPUIdle(s_Gfx);
    GPUTexture2D_Destroy(s_Tex0);
    GPUTextureSampler_Destroy(s_Sampler0);
    s_Material0.DestoryEverything();
    delete g_FramebufferReserve;
    delete s_Gfx;
}