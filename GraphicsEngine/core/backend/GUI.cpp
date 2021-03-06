#include "GUI.h"
#include "backend/VkGraphicsCard.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

static VkDescriptorPool ImGuiPool;
static VkDevice g_Device;

void Gui_VKInitalizeImGui(VkInstance Instance, VkAllocationCallbacks *allocation_callback, VkPhysicalDevice PhysicalDevice, int QueueFamilyIndex, int SwapchainImageCount, VkDevice device, VkRenderPass renderPass, PlatformWindow *window)
{
#if defined(VK_NO_PROTOTYPES)
    struct ImGUI_LoadFunctions_UserData
    {
        VkInstance instance;
        VkDevice device;
    };
    ImGUI_LoadFunctions_UserData ImGui_Load_UserData;
    ImGui_Load_UserData.instance = Instance;
    ImGui_Load_UserData.device = device;
    ImGui_ImplVulkan_LoadFunctions(
        [](const char *function_name, void *user_data) throw()->PFN_vkVoidFunction
        {
            ImGUI_LoadFunctions_UserData *pUserData = (ImGUI_LoadFunctions_UserData *)user_data;
            PFN_vkVoidFunction proc = (PFN_vkVoidFunction)vkGetDeviceProcAddr(pUserData->device, function_name);
            if (!proc)
                return vkGetInstanceProcAddr(pUserData->instance, function_name);
            else
                return proc;
        },
        &ImGui_Load_UserData);
#endif
    g_Device = device;

    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = 1000;
    createInfo.poolSizeCount = poolSizes.size();
    createInfo.pPoolSizes = poolSizes.data();
    vkcheck(vkCreateDescriptorPool(device, &createInfo, allocation_callback, &ImGuiPool));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    //ImGuiConfigFlags_ViewportsEnable        = 1 << 10,  // Viewport enable flags (require both ImGuiBackendFlags_PlatformHasViewports + ImGuiBackendFlags_RendererHasViewports set by the respective backends)
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

    VkQueue queue;
    vkGetDeviceQueue(device, QueueFamilyIndex, 0, &queue);
    ImGui_ImplGlfw_InitForVulkan(window->GetWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = Instance;
    init_info.PhysicalDevice = PhysicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = QueueFamilyIndex;
    init_info.Queue = queue;
    init_info.PipelineCache = NULL;
    init_info.DescriptorPool = ImGuiPool;
    init_info.Allocator = NULL;
    init_info.MinImageCount = SwapchainImageCount;
    init_info.ImageCount = SwapchainImageCount;
    init_info.CheckVkResultFn = NULL;
    ImGui_ImplVulkan_Init(&init_info, renderPass);
    ImFont* fftt = io.Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCodePL-SemiBold.ttf", 14.5f);

    // Upload Fonts
    {
        // Use any command queue
        VkCommandPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        VkCommandPool command_pool;
		vkCreateCommandPool(device, &createInfo, allocation_callback, &command_pool);
        VkCommandBuffer command_buffer;
        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocInfo.commandPool = command_pool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		vkcheck(vkAllocateCommandBuffers(device, &allocInfo, &command_buffer));

        vkResetCommandPool(device, command_pool, 0);
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        vkEndCommandBuffer(command_buffer);
        vkQueueSubmit(queue, 1, &end_info, NULL);

        vkQueueWaitIdle(queue);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
        vkDestroyCommandPool(device, command_pool, allocation_callback);
    }
}

void Gui_VKBeginGUIFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui_VKEndGUIFrame(VkCommandBuffer cmd)
{
    ImGui::Render();
    //ImGui::UpdatePlatformWindows();
    //ImGui::RenderPlatformWindowsDefault();
    auto draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
}

void Gui_VKDestroyImGui(VkAllocationCallbacks *allocation_callback)
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(g_Device, ImGuiPool, allocation_callback);
}

ImFont* Gui_VkLoadFont(VkDevice device, VkQueue queue, const char* path, float size)
{
    auto io = ImGui::GetIO();
    ImFont *fftt = io.Fonts->AddFontFromFileTTF(path, size);
    // Upload Fonts
    {
        // Use any command queue
        VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        VkCommandPool command_pool;
        vkCreateCommandPool(device, &createInfo, nullptr, &command_pool);
        VkCommandBuffer command_buffer;
        VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandPool = command_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkcheck(vkAllocateCommandBuffers(device, &allocInfo, &command_buffer));

        vkResetCommandPool(device, command_pool, 0);
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        vkEndCommandBuffer(command_buffer);
        vkQueueSubmit(queue, 1, &end_info, NULL);

        vkQueueWaitIdle(queue);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
        vkDestroyCommandPool(device, command_pool, nullptr);
    }
    return fftt;
}
