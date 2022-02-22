#include "Exoab.hpp"
#include "Camera.hpp"
#include "mesh/Map.hpp"
#include "ShaderTypes.hpp"
#include "UI.hpp"
#include <memory/Buffer2.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <backend/VkGraphicsCard.hpp>
#include "mesh/geometry.hpp"
#include <shaders/ShaderBinding.hpp>
#define GLM_GTC_quaternion
#include <glm/gtc/quaternion.hpp>

/*
    *** NOTE *** The file is only used for testing.
*/

constexpr bool Graphics_EnableDebug = true;

static ConfigurationSettings s_Config;
static IGraphics3D s_Gfx;
static PlatformWindow *s_Window;
static vk::VkContext s_Context;
static ITexture2 s_Tex0;
static IGPUTextureSampler s_Sampler0;
static Material s_Material0;
static ImFont* s_Font;
static ShaderTypes::ObjectData* s_ObjectDataPtr[3];
static ShaderTypes::SceneData* s_SceneData[3];
static ShaderTypes::DrawData* s_DrawDataPtr[3];
constexpr uint32_t ss_MaxObjects = 10000;
static VkDescriptorPool s_SetPool;
static VkCommandPool s_Pool0;
static VkCommandBuffer s_Cmd[3];
static IBuffer2 s_VerticesSSBO;
static IBuffer2 s_IndicesBuffer;
static std::vector<Mesh::Geometry> s_Geomtry;
static VkSemaphore s_Semaphores[3];
static VkFence s_Fence[3];
static vk::GraphicsSwapchain s_Swapchain;
static ShaderSet s_Set0;
static ShaderSet s_Set1;
static IBuffer2 s_DrawCountBuffer;

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
        s_Config.FutureFrameCount = 3;
        s_Config.VSync = false;
        s_Gfx = Graphics3D_Create(&s_Config, s_Config.WindowTitle.c_str(), ss_DebugMode, true);
        s_Window = s_Gfx->m_window;
        s_Context = ToVKContext(s_Gfx->m_context);
        s_Swapchain = s_Gfx->m_vswapchain;
    }

    // Configure Shader Cache
    Shader::ConfigureShaderCache(ss_ShaderCache);
    g_FramebufferReserve = new FramebufferReserve(s_Context, config, ss_FramebufferReserve);

    assert(Mesh::LoadVerticesIndicesSSBOs(s_Context, { "assets/mesh/cube.obj", "assets/mesh/ball.obj" }, s_Geomtry, &s_VerticesSSBO, &s_IndicesBuffer));

    std::vector<ShaderBinding> bindings(4);
    bindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
    bindings[0].m_bindingID = 0;
    bindings[0].m_hostvisible = true;
    bindings[0].m_useclientbuffer = true;
    bindings[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].m_size = 0;
    bindings[0].m_client_buffer = s_VerticesSSBO;

    bindings[1] = bindings[0];
    bindings[1].m_useclientbuffer = false;
    bindings[1].m_type = SHADER_BINDING_UNIFORM_BUFFER;
    bindings[1].m_bindingID = 1;
    bindings[1].m_size = sizeof(ShaderTypes::SceneData);

    bindings[2] = bindings[1];
    bindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
    bindings[2].m_bindingID = 2;
    bindings[2].m_size = ss_MaxObjects * sizeof(ShaderTypes::ObjectData);

    bindings[3] = bindings[2];
    bindings[3].m_bindingID = 3;
    bindings[3].m_additional_buffer_flags = BufferType::IndirectBuffer;
    bindings[3].m_size = ss_MaxObjects * sizeof(ShaderTypes::DrawData);

    s_Tex0 = Texture2_CreateFromFile(s_Context, "assets/textures/statue.jpg", true);
    s_Sampler0 = GPUTextureSampler_CreateDefaultSampler(s_Context);

    std::vector<ShaderBinding> set1Bindings(1);
    set1Bindings[0].m_type = SHADER_BINDING_COMBINED_TEXTURE_SAMPLER;
    set1Bindings[0].m_bindingID = 0;
    set1Bindings[0].m_hostvisible = false;
    set1Bindings[0].m_useclientbuffer = false;
    set1Bindings[0].m_shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT;
    set1Bindings[0].m_size = 0;
    set1Bindings[0].m_sampler.push_back((VkSampler)s_Sampler0->m_NativeHandle);
    set1Bindings[0].m_textures.push_back(s_Tex0);
    set1Bindings[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    std::vector<VkDescriptorPoolSize> poolSizes;
    ShaderBinding_CalculatePoolSizes(s_Context->FrameInfo->m_FrameCount, poolSizes, bindings);
    ShaderBinding_CalculatePoolSizes(s_Context->FrameInfo->m_FrameCount, poolSizes, set1Bindings);
    s_SetPool = vk::Gfx_CreateDescriptorPool(s_Context, 2 * s_Context->FrameInfo->m_FrameCount, poolSizes);

    s_Set0 = ShaderBinding_Create(s_Context, s_SetPool, 0, bindings);
    s_Set1 = ShaderBinding_Create(s_Context, s_SetPool, 1, set1Bindings);
    VkPipelineLayout vlayout = ShaderBinding_CreatePipelineLayout(s_Context, { s_Set0, s_Set1 }, {});

    MaterialConfiguration uniform_material_mc = MaterialConfiguration("assets/materials/uniforms.mc");

    PipelineVertexInputDescription input_description;
    IPipelineShaders shaders = Material_CreatePipelineShaders(s_Context, &uniform_material_mc);
    IMaterialPipelineLayout layout = Material_CreatePipelineLayout(s_Context, vlayout, input_description, shaders);
    IFramebufferStateManagement state_managment = Material_CreateFramebufferStateManagment(s_Context, &uniform_material_mc, g_FramebufferReserve);
    IMaterialFramebuffer framebuffer = Material_CreateFramebuffer(s_Context, &uniform_material_mc, config, state_managment, g_FramebufferReserve);
    IPipelineState pipeline = Material_CreatePipelineState(s_Context, &uniform_material_mc, layout, state_managment);

    s_Material0.Initalize(shaders, layout, state_managment, framebuffer, pipeline);
    for (unsigned int j = 0; j < s_Context->FrameInfo->m_FrameCount; j++)
    {
        s_SceneData[j] = (ShaderTypes::SceneData*)Buffer2_Map(bindings[1].m_ssbo[j]);
        s_ObjectDataPtr[j] = (ShaderTypes::ObjectData*)Buffer2_Map(bindings[2].m_ssbo[j]);
        s_DrawDataPtr[j] = (ShaderTypes::DrawData*)Buffer2_Map(bindings[3].m_ssbo[j]);
    }

    s_Pool0 = vk::Gfx_CreateCommandPool(s_Context, true, true, false);
    s_Cmd[0] = vk::Gfx_AllocCommandBuffer(s_Context, s_Pool0, true);
    s_Cmd[1] = vk::Gfx_AllocCommandBuffer(s_Context, s_Pool0, true);
    s_Cmd[2] = vk::Gfx_AllocCommandBuffer(s_Context, s_Pool0, true);

    for(unsigned int j = 0; j < s_Context->FrameInfo->m_FrameCount; j++)
    for (int i = 0; i < s_Geomtry.size(); i++) {
        s_DrawDataPtr[j][i].command.indexCount = s_Geomtry[i].indicesCount;
        s_DrawDataPtr[j][i].command.instanceCount = 1;
        s_DrawDataPtr[j][i].command.firstIndex = s_Geomtry[i].firstIndex;
        s_DrawDataPtr[j][i].command.vertexOffset = s_Geomtry[i].firstVertex;
        s_DrawDataPtr[j][i].command.firstInstance = 0;
        s_DrawDataPtr[j][i].ObjectDataIndex = i;
    }
  
    s_Semaphores[0] = vk::Gfx_CreateSemaphore(s_Context, false);
    s_Semaphores[1] = vk::Gfx_CreateSemaphore(s_Context, false);
    s_Semaphores[2] = vk::Gfx_CreateSemaphore(s_Context, false);
    s_Fence[0] = vk::Gfx_CreateFence(s_Context);
    s_Fence[1] = vk::Gfx_CreateFence(s_Context);
    s_Fence[2] = vk::Gfx_CreateFence(s_Context);
    
    UI::Initalize(s_Context, s_Gfx);

    s_DrawCountBuffer = Buffer2_Create(s_Context, BufferType::IndirectBuffer, sizeof(uint32_t), BufferMemoryType::GPU_ONLY);
    uint32_t DrawCount = s_Geomtry.size();
    Buffer2_UploadData(s_DrawCountBuffer, (char8_t*)& DrawCount, 0, sizeof(uint32_t));

    return true;
}

static Camera s_Camera({0, 0, 0});

void BuildCommandBuffers(double dTimeStart);

glm::vec4 ConvertQuaternion(float theta, float i, float j, float k)
{
    return glm::vec4(cos(theta / 2.0), glm::vec3(i, j, k) * sinf(theta / 2.0));
}

void Exoab_Render(double dTimeStart, double dElapsedTime, double FrameRate)
{
    PROFILE_FUNCTION();
    if (s_Window->GetWidth() == 0 || s_Window->GetHeight() == 0)
        return; // window is minimized cannot create a swapchain size 0,0
    VkSemaphore SwapchainReady;
    if (!s_Swapchain.PrepareNextFrame(s_Context->pFrameIndex, &SwapchainReady))
        return;
    uint32_t FrameIndex = *s_Context->pFrameIndex;
    vkcheck(vkWaitForFences(s_Context->defaultDevice, 1, &s_Fence[FrameIndex], true, UINT64_MAX));
    vkResetFences(s_Context->defaultDevice, 1, &s_Fence[FrameIndex]);

    s_ObjectDataPtr[FrameIndex][0].m_Model = glm::mat4(1.0);//glm::vec4(0.0, 0.0, -5.0, 0);
    s_ObjectDataPtr[FrameIndex][0].m_NormalModel = glm::mat4(1.0);// glm::vec4(0.47, -0.88, 0.0, 0.0);

    s_ObjectDataPtr[FrameIndex][1].m_Model = glm::mat4(1.0);// vec4(0., 3.0f, 0., -2.);
    s_ObjectDataPtr[FrameIndex][1].m_NormalModel = glm::mat4(1.0);

    s_SceneData[FrameIndex][0].m_View = s_Camera.GetViewMatrix();
    s_SceneData[FrameIndex][0].m_Projection = (glm::perspective(glm::radians(90.0f), (float)s_Window->m_width / (float)s_Window->m_height, 0.1f, 2000.f));

    BuildCommandBuffers(rand() / (double)RAND_MAX);

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
    vkQueueSubmit(s_Context->defaultQueue, 1, &submitInfo, s_Fence[FrameIndex]);

    UI::FrameRate = FrameRate;
    UI::RenderUI();
    auto i = *s_Context->pFrameIndex;
    s_Swapchain.Present(s_Material0.m_framebuffer->m_textures[UI::ShowDepthBuffer]->m_images[i], s_Material0.m_framebuffer->m_textures[UI::ShowDepthBuffer]->m_views[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, &s_Semaphores[i], false);
}

void BuildCommandBuffers(double dTimeStart)
{
    PROFILE_FUNCTION();

    VkCommandBuffer cmd = s_Cmd[*s_Context->pFrameIndex];
    vkResetCommandBuffer(cmd, 0);
    vk::Gfx_StartCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    
    auto& attachments = s_Material0.m_state_managment->m_attachments;
    VkClearValue pClearValues[2];
    pClearValues[0] = attachments[0].clearColor;
    pClearValues[1] = s_Material0.m_state_managment->m_depth_attachment.value().clearColor;
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
    BeginInfo.clearValueCount = 2;
    BeginInfo.pClearValues = pClearValues;
    {
        PROFILE_SCOPE("[Vulkan] Recording Commands.");
        vkCmdBeginRenderPass(cmd, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)s_Material0.m_pipeline_state->m_pipeline);

    VkDescriptorSet sets[2] = { s_Set0->m_set[*s_Context->pFrameIndex], s_Set1->m_set[*s_Context->pFrameIndex] };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Material0.m_pipeline_layout->m_layout, 0, 2, sets, 0, nullptr);

    vkCmdBindIndexBuffer(cmd, s_IndicesBuffer->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT16);
    // todo: switch to count.
    vkCmdDrawIndexedIndirectCount(cmd, s_Set0->m_bindings[3].m_ssbo[*s_Context->pFrameIndex]->m_vk_buffer->m_buffer, 0, s_DrawCountBuffer->m_vk_buffer->m_buffer, 0, s_Geomtry.size(), sizeof(ShaderTypes::DrawData));

    glm::vec3 LightDirection = glm::vec3(0, 1, 0);
    vkCmdEndRenderPass(cmd);
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
        s_Material0.m_pipeline_state = PipelineState_Create(s_Context, spec, s_Material0.m_state_managment, s_Material0.m_pipeline_layout->input_description, s_Material0.m_pipeline_layout->m_layout, s_Material0.m_pipeline_shaders->m_vertex_shader, s_Material0.m_pipeline_shaders->m_fragment_shader);
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
    GPUTextureSampler_Destroy(s_Sampler0);
    Buffer2_Destroy(s_VerticesSSBO);
    Buffer2_Destroy(s_IndicesBuffer);
    Buffer2_Destroy(s_DrawCountBuffer);
    vkDestroyPipelineLayout(s_Context->defaultDevice, s_Material0.m_pipeline_layout->m_layout, nullptr);
    vkDestroySemaphore(s_Context->defaultDevice, s_Semaphores[0], nullptr);
    vkDestroySemaphore(s_Context->defaultDevice, s_Semaphores[1], nullptr);
    vkDestroySemaphore(s_Context->defaultDevice, s_Semaphores[2], nullptr);
    Texture2_Destroy(s_Tex0);
    ShaderBinding_DestroySets(s_Context, { s_Set0, s_Set1 });
    vkDestroyDescriptorPool(s_Context->defaultDevice, s_SetPool, nullptr);
    vkDestroyCommandPool(s_Context->defaultDevice, s_Pool0, nullptr);
    s_Material0.DestoryEverything();
    delete g_FramebufferReserve;
    Graphics3D_Destroy(s_Gfx);
}