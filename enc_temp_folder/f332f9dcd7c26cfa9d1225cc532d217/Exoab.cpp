#include "Exoab.hpp"
#include "Camera.hpp"
#include "graphics/material_system/ShaderProgramData.hpp"
#include "units/Entity.hpp"
#include "units/Map.hpp"
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
static DefaultEntity* s_Cube;
static IBuffer2 s_InstanceBuffer;
static IShaderProgramData s_ProgramData0;
static IGPUTexture2D s_Tex0;
static IGPUTextureSampler s_Sampler0;
static Material s_Material0;
static ImFont* s_Font;

static bool s_ShowDepthBuffer = false;

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

    s_Cube = Entity_CreateDefaultEntity(s_Context, "assets/mesh/cube.obj");
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

    s_Pool = CommandPool::Create(s_Context, true, true);
    s_Cmd = CommandList_Create(s_Context, &s_Pool);

    s_Tex0 = GPUTexture2D_CreateFromFile(s_Context, "assets/textures/statue.jpg", true);
    s_Sampler0 = GPUTextureSampler_CreateDefaultSampler(s_Context);

    s_ProgramData0 = ShaderProgramData_Create(s_Material0.m_pipeline_layout->m_layout);
    ShaderProgramData_SetConstantTextureArray(s_ProgramData0, 1, 0, s_Sampler0, { s_Tex0 });
    ShaderProgramData_SetConstantSSBO(s_ProgramData0, 0, 0, s_Cube->m_model->GetRenderState(0).m_VertexBuffer);
  
    s_Semaphore = GPUSemaphore_Create(s_Context, false);

    return true;
}

static Camera s_Camera({0, 0, 0});

void BuildCommandBuffers(double dTimeStart);

void Exoab_Render(double dTimeStart, double dElapsedTime)
{
    PROFILE_FUNCTION();
    if (s_Window->GetWidth() == 0 || s_Window->GetHeight() == 0)
        return; // window is minimized cannot create a swapchain size 0,0
    IGPUSemaphore SwapchainReady;
    Graphics3D_PrepareNextFrame(s_Gfx, &SwapchainReady);
    
    BuildCommandBuffers(0);

    Graphics3D_ExecuteCommandList(s_Gfx, s_Cmd, nullptr, 1, &SwapchainReady, 1, &s_Semaphore);
    Exoab_GUI();
    Graphics3D_Present(s_Gfx, s_Material0.m_framebuffer->m_framebuffer, s_ShowDepthBuffer ? 1 : 0, 1, &s_Semaphore, s_ShowDepthBuffer);
    Graphics3D_WaitGPUIdle(s_Gfx);
}

void BuildCommandBuffers(double dTimeStart)
{
    PROFILE_FUNCTION();
    glm::mat4 data[2];
    data[0] = glm::mat4(1.0);
    data[1] = glm::perspective(glm::radians(50.0f), 800.0f / 600.0f, 1.0f, 1950.0f);
    glm::mat4 model[2];
    model[0] = glm::translate(glm::mat4(1), glm::vec3(0.0, 0.0, -5.0)) * glm::rotate(glm::mat4(1.0), (float)dTimeStart, glm::vec3(0., 1.0f, 0.));
    model[1] = glm::transpose(glm::inverse(model[0]));

    CommandList_Reset(s_Cmd, false);
    CommandList_StartRecording(s_Cmd, true, false);

    DefaultEntity* entities[1] = { s_Cube };
    auto& render_state = s_Cube->m_model->GetRenderState(0);

    CommandList_SetFramebuffer(s_Cmd, s_Material0.m_state_managment, s_Material0.m_framebuffer->m_framebuffer);
    CommandList_BindPipeline(s_Cmd, s_Material0.m_pipeline_state);
    ShaderProgramData_UpdateEntityBindingData(s_ProgramData0, s_Material0.m_pipeline_layout->m_layout, 1, (void**)&entities);
    CommandList_SetRenderState(s_Cmd, &render_state, nullptr);
    CommandList_DrawIndexed(s_Cmd, s_Material0.m_pipeline_layout->m_layout, s_ProgramData0, 0, 0, render_state.m_IndicesCount, 1);

    glm::vec3 LightDirection = glm::vec3(0, 1, 0);

    CommandList_StopRecording(s_Cmd);
    IShaderProgramData program_data[1] = { s_ProgramData0 };
    ShaderProgramData_FlushShaderProgramData(1, program_data);
}

void Exoab_GUI()
{
    PROFILE_FUNCTION();
    //ImGui::PushFont(s_Font);
    ImGui::Begin("ImGui");
    ImGui::Text("Im Graphics!");
    ImGui::Button("Accept Graphics");
    if (ImGui::Button("Toggle Wireframe View"))
    {
        CheckActionTime(Graphics3D_WaitGPUIdle(s_Gfx));
        auto spec = s_Material0.m_pipeline_state->m_spec;
        if (spec.m_PolygonMode == PolygonMode::LINE)
            spec.m_PolygonMode = PolygonMode::FILL;
        else
            spec.m_PolygonMode = PolygonMode::LINE;
        PipelineState_Destroy(s_Material0.m_pipeline_state);
        s_Material0.m_pipeline_state = PipelineState_Create(s_Context, spec, s_Material0.m_state_managment, s_Material0.m_pipeline_layout->m_layout, s_Material0.m_pipeline_shaders->m_vertex_shader, s_Material0.m_pipeline_shaders->m_fragment_shader);
    }
    if (s_ShowDepthBuffer) {
        if (ImGui::Button("Show Color Buffer"))
        {
            s_ShowDepthBuffer = false;
        }
    }
    else {
        if (ImGui::Button("Show Depth Buffer"))
        {
            s_ShowDepthBuffer = true;
        }
    }
    //ImGui::PopFont();
    ImGui::End();
}

Tristate Exoab_Update(double dTimeStart, double dElapsedTime)
{
    PROFILE_FUNCTION();
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