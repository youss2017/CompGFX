#include "UI.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <backend/GUI.h>
#include "utils/Profiling.hpp"
#include "graphics/Graphics.hpp"
#include <imgui.h>
#include <string>
#include <sstream>

namespace UI
{
	static ImFont* s_Font0; // CascadiaCode
    static GraphicsContext s_Context;
    static Graphics3D* s_Gfx;

    extern bool StateChanged = false;
    bool ShowDepthBuffer = false;
    bool ShowWireframe = false;
    double FrameRate = 0.0;
    float C_x, C_y, C_z;
}

void UI::Initalize(void* _context, void* _gfx)
{
	vk::VkContext context = ToVKContext(_context);
	s_Font0 = Gui_VkLoadFont(context->defaultDevice, context->defaultQueue, "assets/fonts/CascadiaCodePL-SemiBold.ttf", 14.5f);
    s_Context = _context;
    s_Gfx = (Graphics3D*)_gfx;
}

void UI::RenderUI()
{
    PROFILE_FUNCTION();
    ImGui::Begin("ImGui");
    ImGui::PushFont(s_Font0);
    ImGui::Text("Im Graphics!");
    if (ImGui::Button("Toggle Wireframe View"))
    {
        StateChanged = true;
        UI::ShowWireframe = !UI::ShowWireframe;
    }
    if (UI::ShowDepthBuffer) {
        if (ImGui::Button("Show Color Buffer"))
        {
            UI::ShowDepthBuffer = false;
        }
    }
    else {
        if (ImGui::Button("Show Depth Buffer"))
        {
            UI::ShowDepthBuffer = true;
        }
    }
    ImGui::PopFont();
    ImGui::End();
    int corner = 0;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
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
    ImGui::PushFont(s_Font0);
    ImGui::Text("Overlay");
    ImGui::Separator();
    ImGui::Text("%.4f FPS", FrameRate);
    ImGui::Text("<%.2f, %.2f, %.2f>", C_x, C_y, C_z);
    std::stringstream ss;
    ss << FrameRate << " FPS";
    ImGui::Text(ss.str().c_str());
    ImGui::PopFont();
    ImGui::End();
}
