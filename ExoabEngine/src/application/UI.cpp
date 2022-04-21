#include "UI.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <backend/GUI.h>
#include "../utils/Profiling.hpp"
#include "../graphics/Graphics.hpp"
#include <imgui.h>
#include <string>
#include <sstream>
#include "Globals.hpp"

namespace UI
{
	static ImFont* s_Font0; // CascadiaCode
    static vk::VkContext s_Context;
    static Graphics3D* s_Gfx;

    extern bool StateChanged = false;
    bool ShowWireframe = false;
    bool LockFrustrum = false;
    double FrameRate = 0.0;
    double FrameTime = 0.0;
    double FrustrumCullingTime = 0.0;
    double BloomPassTime = 0.0;
    double ShadowPassTime = 0.0;
    double DebugPassTime = 0.0;
    double NextFrameTime = 0.0;
    unsigned long long FrustrumInvocations = 0;
    double GeometryPassTime = 0;
    unsigned long long VertexInvocations = 0;
    unsigned long long FragmentInvocations = 0;
    int CubemapLOD = 0;
    int CubemapLODMax = 0;
    int CurrentOutputBuffer = 0;
    glm::vec3 CameraPosition;
    glm::vec3 LightPosition;
    static bool UIInDebugMode = false;
    bool VSync = true;
    bool ReloadShaders = false;
    bool ClearShaderCache = false;
    int BloomDownsampleMip = 0;
    int Frequency[3];
    int Octave[3];
    int Octave1;
    int Octave2;
    bool RegenerateNoiseMap = false;
    static ImTextureID NoiseMapTexture = nullptr;
    static ImTextureID NoiseMapTexture1 = nullptr;
    static ImTextureID NoiseMapTexture2 = nullptr;
    int ActiveNoiseMap = 0;
    float Contribution[3] = { 1.0 };
    bool SaveTerrain = false;
    bool ShowStatistics = false;
    float Exposure = 1.0f;
}

void UI::Initalize(void* _context, void* _gfx)
{
	vk::VkContext context = (vk::VkContext)(_context);
    s_Context = context;
    s_Gfx = (Graphics3D*)_gfx;
}

void UI::UpdateNoiseMap(ITexture2 noise1, ITexture2 noise2, ITexture2 noise3, VkSampler sampler) {
    NoiseMapTexture = ImGui_ImplVulkan_AddTexture(sampler, noise1->m_vk_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    NoiseMapTexture1 = ImGui_ImplVulkan_AddTexture(sampler, noise2->m_vk_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    NoiseMapTexture2 = ImGui_ImplVulkan_AddTexture(sampler, noise3->m_vk_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void UI::RenderUI()
{
    PROFILE_FUNCTION();
    int corner = 1;
    if (UIInDebugMode)
        corner = 1;
    else corner = 0;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (ShowStatistics) {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::Begin("Information Overlay", nullptr, window_flags);
        ImGui::Text("Overlay");
        ImGui::Separator();
        ImGui::Text("%.2f FPS", Global::FrameRate);
        ImGui::SameLine();
        ImGui::Text("%.2f ms -- Frame Time", FrameTime);
        ImGui::SameLine();
        if (ImGui::Checkbox("V-Sync", &VSync)) {
            StateChanged = true;
        }
        double cpuTime = FrameTime - ShadowPassTime - FrustrumCullingTime - GeometryPassTime - DebugPassTime - BloomPassTime - NextFrameTime;
        ImGui::Text("%.2f ms -- CPU Time", cpuTime);
        ImGui::Text("%llu / %llu -- Vertex/Fragment Invocations", VertexInvocations, FragmentInvocations);
        ImGui::Text("%.2f ms -- Shadow Map", ShadowPassTime);
        ImGui::Text("%.2f ms / %llu -- (Geo) Frustrum Pass", FrustrumCullingTime, FrustrumInvocations);
        ImGui::Text("%.2f ms -- Geometry Pass", GeometryPassTime);
        ImGui::Text("%.2f ms -- Debug Pass", DebugPassTime);
        ImGui::Text("%.2f ms -- Bloom Pass", BloomPassTime);
        ImGui::Text("<%.2f, %.2f, %.2f>", CameraPosition[0], CameraPosition[1], CameraPosition[2]);
        ImGui::End();
    }
    ImGui::Begin("Control Panel");
    ImGui::SliderFloat3("Light", &LightPosition[0], -100.0, 100.0f);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        LightPosition = glm::vec3(0.0f);
    }
    //ImGui::SliderFloat("Exposure", &Exposure, 0.0, 10.0);
    static char buf[50]{ "1.0" };
    ImGui::InputText("Exposure", buf, 50);
    try {
        float exposure = std::stof(std::string(buf));
        UI::Exposure = exposure;
    }
    catch (std::exception& e) {}
    ImGui::SliderInt("Cubemap LOD", &UI::CubemapLOD, 0, UI::CubemapLODMax);
    ImGui::SliderInt("Bloom Downsample", &UI::BloomDownsampleMip, 0, 7);
    const char* items[] = { "Color Buffer", "Depth Buffer", "Shadow Buffer", "Bloom Buffer" };
    ImGui::Combo("Output Buffer", &CurrentOutputBuffer, items, IM_ARRAYSIZE(items));
    if (ImGui::Button("Toggle Wireframe View"))
    {
        StateChanged = true;
        UI::ShowWireframe = !UI::ShowWireframe;
    }
    ImGui::SameLine();
    if (UI::LockFrustrum) {
        if (ImGui::Button("Unlock Frustrum"))
            UI::LockFrustrum = false;
    }
    else {
        if (ImGui::Button("Lock Frustrum"))
            UI::LockFrustrum = true;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &UIInDebugMode);
    ImGui::SameLine();
    ImGui::Checkbox("Show Metrics", &UI::ShowStatistics);
    if (ImGui::Button("Reload Shaders")) {
        StateChanged = true;
        ReloadShaders = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Shader Cache")) {
        StateChanged = true;
        ClearShaderCache = true;
    }
    if (NoiseMapTexture) {
        if(ImGui::ImageButton(NoiseMapTexture, ImVec2(100, 100))  ) {ActiveNoiseMap = 0;} ImGui::SameLine();
        if(ImGui::ImageButton(NoiseMapTexture1, ImVec2(100, 100)) ) {ActiveNoiseMap = 1;} ImGui::SameLine();
        if (ImGui::ImageButton(NoiseMapTexture2, ImVec2(100, 100))) { ActiveNoiseMap = 2; } ImGui::SameLine();
        ImGui::Text("Active Noise Map %d", ActiveNoiseMap);
        ImGui::Text("Contributions: %.2f, %.2f, %.2f Total: %.2f%%", Contribution[0], Contribution[1], Contribution[2], (Contribution[0] + Contribution[1] + Contribution[2]) * 100.0);
        ImGui::SliderInt("Frequency", &Frequency[ActiveNoiseMap], 0.0, 64.0f);
        ImGui::SliderInt("Octaves", &Octave[ActiveNoiseMap], 0, 16);
        ImGui::SliderFloat("Contribution", &Contribution[ActiveNoiseMap], 0.0, 1.0f);
        if (ImGui::Button("Regenerate")) {
            RegenerateNoiseMap = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Terrain")) {
        StateChanged = true;
        SaveTerrain = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Quit")) {
        Global::Quit = true;
    }
    ImGui::End();
    //ImGui::ShowDemoWindow();
}
