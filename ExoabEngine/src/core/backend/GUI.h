#pragma once
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>

class PlatformWindow;

void Gui_VKInitalizeImGui(VkInstance Instance, VkAllocationCallbacks *allocation_callback, VkPhysicalDevice PhysicalDevice, int QueueFamilyIndex, int SwapchainImageCount, VkDevice device, VkRenderPass renderPass, PlatformWindow *window);
void Gui_VKBeginGUIFrame();
void Gui_VKEndGUIFrame(VkCommandBuffer cmd);

void Gui_GLInitalizeImGui(PlatformWindow* window);
void Gui_GLEndGUIFrame();

ImFont* Gui_VkLoadFont(VkDevice device, VkQueue queue, const char* path, float size);

void Gui_VKDestroyImGui(VkAllocationCallbacks* allocation_callback);

void Gui_GLBeginGUIFrame();
void Gui_GLDestroyImGui();