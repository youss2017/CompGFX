#include "Exoab.hpp"
#include "Camera.hpp"
#include "graphics/material_system/ShaderProgramData.hpp"
#include "units/Entity.hpp"
#include "units/Map.hpp"
#include "ShaderTypes.hpp"
#include "UI.hpp"
#include <memory/Buffer2.hpp>

/*
    *** NOTE *** The file is only used for testing.
*/

constexpr bool Graphics_EnableDebug = true;

static ConfigurationSettings s_Config;
static IGraphics3D s_Gfx;
static PlatformWindow *s_Window;
static GraphicsContext s_Context;
static CommandPool s_Pool;
static ICommandList s_Cmd;
static IGPUSemaphore s_Semaphore;
static IBuffer2 s_InstanceBuffer;
static IShaderProgramData s_ProgramData0;
static IGPUTexture2D s_Tex0;
static IGPUTextureSampler s_Sampler0;
static Material s_Material0;
static ImFont* s_Font;
static IBuffer2 s_ObjectDataBuffer;
static IBuffer2 s_DrawDataBuffer;
static ShaderTypes::ObjectData* s_ObjectDataPtr;
static ShaderTypes::DrawData* s_DrawDataPtr;
constexpr uint32_t ss_MaxObjects = 10000;

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
        s_Context = s_Gfx->m_context;
    }

    // Configure Shader Cache
    Shader::ConfigureShaderCache(ss_ShaderCache);
    g_FramebufferReserve = new FramebufferReserve(s_Context, config, ss_FramebufferReserve);

    MaterialConfiguration uniform_material_mc = MaterialConfiguration("assets/materials/uniforms.mc");

    PipelineVertexInputDescription input_description;
    //input_description.AddInputElement("inPosition", 0, 0, 3, true, false, false);
    //input_description.AddInputElement("inNormal", 1, 0, 3, true, false, false);
    //input_description.AddInputElement("inTexCoord", 2, 0, 2, true, false, false);
    //input_description.AddInputElement("XYOffset", 3, 1, 2, true, false, true, 1u);
    
    IPipelineShaders shaders = Material_CreatePipelineShaders(s_Context, &uniform_material_mc);
    IMaterialPipelineLayout layout = Material_CreatePipelineLayout(s_Context, input_description, shaders);
    IFramebufferStateManagement state_managment = Material_CreateFramebufferStateManagment(s_Context, &uniform_material_mc, g_FramebufferReserve);
    IMaterialFramebuffer framebuffer = Material_CreateFramebuffer(s_Context, &uniform_material_mc, config, state_managment, g_FramebufferReserve);
    IPipelineState pipeline = Material_CreatePipelineState(s_Context, &uniform_material_mc, layout, state_managment);

    s_Material0.Initalize(shaders, layout, state_managment, framebuffer, pipeline);
    s_ObjectDataBuffer = Buffer2_Create(s_Context, BufferType::StorageBuffer, sizeof(ShaderTypes::ObjectData) * ss_MaxObjects, BufferMemoryType::CPU_TO_CPU);
    s_DrawDataBuffer = Buffer2_Create(s_Context, BufferType(StorageBuffer | IndirectBuffer), sizeof(ShaderTypes::DrawData) * ss_MaxObjects, BufferMemoryType::CPU_TO_CPU);
    s_ObjectDataPtr = (ShaderTypes::ObjectData*)Buffer2_Map(s_ObjectDataBuffer, false, true);
    s_DrawDataPtr = (ShaderTypes::DrawData*)Buffer2_Map(s_DrawDataBuffer, false, true);

    s_Pool = CommandPool::Create(s_Context, true, true);
    s_Cmd = CommandList_Create(s_Context, &s_Pool);

    s_Tex0 = GPUTexture2D_CreateFromFile(s_Context, "assets/textures/statue.jpg", true);
    s_Sampler0 = GPUTextureSampler_CreateDefaultSampler(s_Context);

    s_ProgramData0 = ShaderProgramData_Create(s_Material0.m_pipeline_layout->m_layout);
    ShaderProgramData_SetConstantTextureArray(s_ProgramData0, 1, 0, s_Sampler0, { s_Tex0 });
    ShaderProgramData_SetConstantSSBO(s_ProgramData0, 0, 0, s_Cube->m_model->GetRenderState(0).m_VertexBuffer);
    ShaderProgramData_SetConstantSSBO(s_ProgramData0, 0, 1, s_Cube->m_model->GetRenderState(0).m_IndexBuffer);
    ShaderProgramData_SetConstantSSBO(s_ProgramData0, 0, 2, s_ObjectDataBuffer);
    ShaderProgramData_SetConstantSSBO(s_ProgramData0, 0, 3, s_DrawDataBuffer);

    //s_DrawDataPtr[0].command.firstIndex= 0;
    //s_DrawDataPtr[0].command.firstInstance  = 0;
    //s_DrawDataPtr[0].command.indexCount = s_Cube->m_model->GetIndicesCount(0);
    //s_DrawDataPtr[0].command.instanceCount = 1;
    //s_DrawDataPtr[0].command.vertexOffset = 0;
    s_DrawDataPtr[0].command.firstInstance = 0;
    s_DrawDataPtr[0].command.firstVertex = 0;
    s_DrawDataPtr[0].command.instanceCount = 1;
    s_DrawDataPtr[0].command.vertexCount = s_Cube->m_model->GetIndicesCount(0);
    s_DrawDataPtr[0].ObjectDataIndex = 0;
  
    s_Semaphore = GPUSemaphore_Create(s_Context, false);
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
    IGPUSemaphore SwapchainReady;
    Graphics3D_PrepareNextFrame(s_Gfx, &SwapchainReady);
    
    glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(0.0, 0.0, -5.0)) * glm::rotate(glm::mat4(1.0), (float)dTimeStart, glm::vec3(0., 1.0f, 0.));
    s_ObjectDataPtr[0].m_Model = model;
    s_ObjectDataPtr[0].m_NormalModel = glm::transpose(glm::inverse(model));
    s_ObjectDataPtr[0].m_View = s_Camera.GetViewMatrix();
    s_ObjectDataPtr[0].m_Projection = (glm::perspectiveRH(glm::radians(90.0f), (float)s_Window->m_width / (float)s_Window->m_height, 0.1f, 2000.f));
    Buffer2_Flush(s_ObjectDataBuffer, 0, sizeof(ShaderTypes::ObjectData));

    BuildCommandBuffers(0);

    Graphics3D_ExecuteCommandList(s_Gfx, s_Cmd, nullptr, 1, &SwapchainReady, 1, &s_Semaphore);
    UI::FrameRate = FrameRate;
    UI::RenderUI();
    Graphics3D_Present(s_Gfx, s_Material0.m_framebuffer->m_framebuffer, UI::ShowDepthBuffer ? 1 : 0, 1, &s_Semaphore, UI::ShowDepthBuffer);
    Graphics3D_WaitGPUIdle(s_Gfx);
}

void BuildCommandBuffers(double dTimeStart)
{
    PROFILE_FUNCTION();

    CommandList_Reset(s_Cmd, false);
    CommandList_StartRecording(s_Cmd, true, false);

    DefaultEntity* entities[1] = { s_Cube };
    auto& render_state = s_Cube->m_model->GetRenderState(0);

    CommandList_SetFramebuffer(s_Cmd, s_Material0.m_state_managment, s_Material0.m_framebuffer->m_framebuffer);
    CommandList_BindPipeline(s_Cmd, s_Material0.m_pipeline_state);
    ShaderProgramData_UpdateEntityBindingData(s_ProgramData0, s_Material0.m_pipeline_layout->m_layout, 1, (void**)&entities);
    s_ObjectDataPtr[0].m_Projection = (glm::perspectiveRH(glm::radians(45.0f), (float)s_Window->m_width / (float)s_Window->m_height, 0.1f, 2000.f));
    CommandList_DrawIndexed(s_Cmd, s_Material0.m_pipeline_layout->m_layout, s_ProgramData0, 0, 0, render_state.m_IndicesCount, 1);
    vkCmdDrawIndirect(VkCommandList_Get(s_Cmd), s_DrawDataBuffer->m_vk_buffer->m_buffer, 0, 1, sizeof(ShaderTypes::DrawData));

    glm::vec3 LightDirection = glm::vec3(0, 1, 0);

    CommandList_StopRecording(s_Cmd);
    IShaderProgramData program_data[1] = { s_ProgramData0 };
    ShaderProgramData_FlushShaderProgramData(1, program_data);
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
    ShaderProgramData_Destroy(s_ProgramData0);
    CommandList_Destroy(s_Cmd, nullptr, false);
    s_Pool.Destroy();
    GPUSemaphore_Destroy(s_Semaphore);
    GPUTexture2D_Destroy(s_Tex0);
    GPUTextureSampler_Destroy(s_Sampler0);
    Buffer2_Destroy(s_InstanceBuffer);
    s_Material0.DestoryEverything();
    delete s_Cube;
    delete g_FramebufferReserve;
    delete s_Gfx;
}