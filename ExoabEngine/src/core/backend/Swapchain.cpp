#include "Swapchain.hpp"
#include "../../utils/Profiling.hpp"
#include "../../window/PlatformWindow.hpp"
#include "../backend/VulkanLoader.h"
#include "../../utils/common.hpp"
#include "../shaders/Shader.hpp"
#include "GUI.h"
#include <vector>
#include <climits>
#include <cassert>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <GLFW/glfw3.h>

/*
    Written: 9/26/2021
    Last Modified: 12/21/2021
    The swapchain is not thread safe.
    Goal of this file is to create and manage Vulkan Swapchain.
    - Must create/destroy swapchain
    - Get Next Frame
    - Copy Color Texture to Swapchain Image
*/

#if defined(_DEBUG)
#define vkcheck(x)\
{\
	VkResult ___vk_i_result____ = x;\
	if(___vk_i_result____  != VK_SUCCESS) {\
		std::string error = "Vulkan-Error: function ---> "; error += #x; error += " failed with error code " + __GetVkResultString(___vk_i_result____);\
		log_error(error.c_str(), __FILE__, __LINE__);\
	}\
}
#else
#define vkcheck(x) x
#endif

static std::string __GetVkResultString(VkResult result)
	{
		if(VK_SUCCESS == result) return std::string("VK_SUCCESS");
		if(VK_NOT_READY == result) return std::string("VK_NOT_READY");
		if(VK_TIMEOUT == result) return std::string("VK_TIMEOUT");
		if(VK_EVENT_SET == result) return std::string("VK_EVENT_SET");
		if(VK_EVENT_RESET == result) return std::string("VK_EVENT_RESET");
		if(VK_INCOMPLETE == result) return std::string("VK_INCOMPLETE");
		if(VK_ERROR_OUT_OF_HOST_MEMORY == result) return std::string("VK_ERROR_OUT_OF_HOST_MEMORY");
		if(VK_ERROR_OUT_OF_DEVICE_MEMORY == result) return std::string("VK_ERROR_OUT_OF_DEVICE_MEMORY");
		if(VK_ERROR_INITIALIZATION_FAILED == result) return std::string("VK_ERROR_INITIALIZATION_FAILED");
		if(VK_ERROR_DEVICE_LOST == result) return std::string("VK_ERROR_DEVICE_LOST");
		if(VK_ERROR_MEMORY_MAP_FAILED == result) return std::string("VK_ERROR_MEMORY_MAP_FAILED");
		if(VK_ERROR_LAYER_NOT_PRESENT == result) return std::string("VK_ERROR_LAYER_NOT_PRESENT");
		if(VK_ERROR_EXTENSION_NOT_PRESENT == result) return std::string("VK_ERROR_EXTENSION_NOT_PRESENT");
		if(VK_ERROR_FEATURE_NOT_PRESENT == result) return std::string("VK_ERROR_FEATURE_NOT_PRESENT");
		if(VK_ERROR_INCOMPATIBLE_DRIVER == result) return std::string("VK_ERROR_INCOMPATIBLE_DRIVER");
		if(VK_ERROR_TOO_MANY_OBJECTS == result) return std::string("VK_ERROR_TOO_MANY_OBJECTS");
		if(VK_ERROR_FORMAT_NOT_SUPPORTED == result) return std::string("VK_ERROR_FORMAT_NOT_SUPPORTED");
		if(VK_ERROR_FRAGMENTED_POOL == result) return std::string("VK_ERROR_FRAGMENTED_POOL");
		if(VK_ERROR_UNKNOWN == result) return std::string("VK_ERROR_UNKNOWN");
		if(VK_ERROR_OUT_OF_POOL_MEMORY == result) return std::string("VK_ERROR_OUT_OF_POOL_MEMORY");
		if(VK_ERROR_INVALID_EXTERNAL_HANDLE == result) return std::string("VK_ERROR_INVALID_EXTERNAL_HANDLE");
		if(VK_ERROR_FRAGMENTATION == result) return std::string("VK_ERROR_FRAGMENTATION");
		if(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS == result) return std::string("VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS");
		if(VK_ERROR_SURFACE_LOST_KHR == result) return std::string("VK_ERROR_SURFACE_LOST_KHR");
		if(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR == result) return std::string("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
		if(VK_SUBOPTIMAL_KHR == result) return std::string("VK_SUBOPTIMAL_KHR");
		if(VK_ERROR_OUT_OF_DATE_KHR == result) return std::string("VK_ERROR_OUT_OF_DATE_KHR");
		if(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR == result) return std::string("VK_ERROR_INCOMPATIBLE_DISPLAY_KHR");
		if(VK_ERROR_VALIDATION_FAILED_EXT == result) return std::string("VK_ERROR_VALIDATION_FAILED_EXT");
		if(VK_ERROR_INVALID_SHADER_NV == result) return std::string("VK_ERROR_INVALID_SHADER_NV");
		if(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT == result) return std::string("VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT");
		if(VK_ERROR_NOT_PERMITTED_EXT == result) return std::string("VK_ERROR_NOT_PERMITTED_EXT");
		if(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT == result) return std::string("VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT");
		if(VK_THREAD_IDLE_KHR == result) return std::string("VK_THREAD_IDLE_KHR");
		if(VK_THREAD_DONE_KHR == result) return std::string("VK_THREAD_DONE_KHR");
		if(VK_OPERATION_DEFERRED_KHR == result) return std::string("VK_OPERATION_DEFERRED_KHR");
		if(VK_OPERATION_NOT_DEFERRED_KHR == result) return std::string("VK_OPERATION_NOT_DEFERRED_KHR");
		if(VK_PIPELINE_COMPILE_REQUIRED_EXT == result) return std::string("VK_PIPELINE_COMPILE_REQUIRED_EXT");
		if(VK_ERROR_OUT_OF_POOL_MEMORY_KHR == result) return std::string("VK_ERROR_OUT_OF_POOL_MEMORY_KHR");
		if(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR == result) return std::string("VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR");
		if(VK_ERROR_FRAGMENTATION_EXT == result) return std::string("VK_ERROR_FRAGMENTATION_EXT");
		if(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT == result) return std::string("VK_ERROR_INVALID_DEVICE_ADDRESS_EXT");
		if(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR == result) return std::string("VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR");
		if(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT == result) return std::string("VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT");
		if(VK_RESULT_MAX_ENUM == result) return std::string("VK_RESULT_MAX_ENUM");
		return std::string("UNDEFEINED or UNDETECTED VKRESULT! VkGraphicsCard.cpp");
	}

static const char *s_VertexShaderSource =
    "#version 450 core\n"
    "const vec4 quad[] = \n"
    "{\n"
    "    // triangle 1\n"
    "    vec4(-1, 1, 0, 1),\n"
    "    vec4(1, -1, 1, 0),\n"
    "    vec4(-1, -1, 0, 0),\n"
    "    // triangle 2\n"
    "    vec4(-1, 1, 0, 1),\n"
    "    vec4(1, 1, 1, 1),\n"
    "    vec4(1, -1, 1, 0)\n"
    "};\n"
    "layout (location = 0) out vec2 TexCoord;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 Vertex = quad[gl_VertexIndex];\n"
    "    gl_Position = vec4(Vertex.xy, 0.0, 1.0);\n"
    "    TexCoord = Vertex.zw;	\n"
    "}\n";

static const char *s_FragmentShaderSource =
    "#version 450 core\n"
    "\n"
    "layout (location = 0) in vec2 TexCoord;\n"
    "layout (location = 0) out vec4 FragColor;\n"
    "layout(set = 0, binding = 0) uniform sampler2D ColorPassTexture;\n"
    "\n"
    "// Do not use this function if the Output Texture is using an SRGBA format\n"
    "vec4 GammaCorrection(vec4 color, float gamma)\n"
    "{\n"
    "    return pow(color, vec4(vec3(1.0 / gamma), 1.0));\n"
    "}\n"
    "    // from http://www.java-gaming.org/index.php?topic=35123.0\n"
    "vec4 cubic(float v){\n"
    "    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;\n"
    "    vec4 s = n * n * n;\n"
    "    float x = s.x;\n"
    "    float y = s.y - 4.0 * s.x;\n"
    "    float z = s.z - 4.0 * s.y + 6.0 * s.x;\n"
    "    float w = 6.0 - x - y - z;\n"
    "    return vec4(x, y, z, w) * (1.0/6.0);\n"
    "}\n"
    "\n"
    "vec4 textureBicubic(sampler2D samp, vec2 texCoords){\n"
    "\n"
    "   vec2 texSize = textureSize(samp, 0);\n"
    "   vec2 invTexSize = 1.0 / texSize;\n"
    "\n"
    "   texCoords = texCoords * texSize - 0.5;\n"
    "\n"
    "\n"
    "    vec2 fxy = fract(texCoords);\n"
    "    texCoords -= fxy;\n"
    "\n"
    "    vec4 xcubic = cubic(fxy.x);\n"
    "    vec4 ycubic = cubic(fxy.y);\n"
    "\n"
    "    vec4 c = texCoords.xxyy + vec2 (-0.5, +1.5).xyxy;\n"
    "\n"
    "    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);\n"
    "    vec4 offset = c + vec4 (xcubic.yw, ycubic.yw) / s;\n"
    "\n"
    "    offset *= invTexSize.xxyy;\n"
    "\n"
    "    vec4 sample0 = texture(samp, offset.xz);\n"
    "    vec4 sample1 = texture(samp, offset.yz);\n"
    "    vec4 sample2 = texture(samp, offset.xw);\n"
    "    vec4 sample3 = texture(samp, offset.yw);\n"
    "\n"
    "    float sx = s.x / (s.x + s.y);\n"
    "    float sy = s.z / (s.z + s.w);\n"
    "\n"
    "    return mix(\n"
    "       mix(sample3, sample2, sx), mix(sample1, sample0, sx)\n"
    "    , sy);\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 Color = texture(ColorPassTexture, TexCoord);\n"
    "    FragColor = Color;\n"
    "}\n";

constexpr bool ss_Compile_SourceCode = false;

static uint32_t s_VertexShaderBytecode[] = 
{
119734787, 66816, 851978, 47, 0, 131089, 1, 393227, 1, 1280527431, 1685353262, 808793134, 0, 196622, 0, 1, 524303, 0, 4, 1852399981, 0, 23, 33, 44, 262215, 23, 11, 42, 196679, 26, 24, 327752, 31, 0, 11, 0, 327752, 31, 1, 11, 1, 327752, 31, 2, 11, 3, 327752, 31, 3, 11, 4, 196679, 31, 2, 262215, 44, 30, 0, 131091, 2, 196641, 3, 2, 196630, 6, 32, 262167, 7, 6, 4, 262176, 8, 7, 7, 262165, 10, 32, 0, 262187, 10, 11, 6, 262172, 12, 7, 11, 262187, 6, 13, 3212836864, 262187, 6, 14, 1065353216, 262187, 6, 15, 0, 458796, 7, 16, 13, 14, 15, 14, 458796, 7, 17, 14, 13, 14, 15, 458796, 7, 18, 13, 13, 15, 15, 458796, 7, 19, 14, 14, 14, 14, 589868, 12, 20, 16, 17, 18, 16, 19, 17, 262165, 21, 32, 1, 262176, 22, 1, 21, 262203, 22, 23, 1, 262176, 25, 7, 12, 262187, 10, 29, 1, 262172, 30, 6, 29, 393246, 31, 7, 6, 30, 30, 262176, 32, 3, 31, 262203, 32, 33, 3, 262187, 21, 34, 0, 262167, 35, 6, 2, 262176, 41, 3, 7, 262176, 43, 3, 35, 262203, 43, 44, 3, 327734, 2, 4, 0, 3, 131320, 5, 327739, 25, 26, 7, 20, 262205, 21, 24, 23, 327745, 8, 27, 26, 24, 262205, 7, 28, 27, 327761, 6, 38, 28, 0, 327761, 6, 39, 28, 1, 458832, 7, 40, 38, 39, 15, 14, 327745, 41, 42, 33, 34, 196670, 42, 40, 458831, 35, 46, 28, 28, 2, 3, 196670, 44, 46, 65789, 65592
};

static uint32_t s_FragmentShaderBytecode[] =
{
119734787, 66816, 851978, 22, 0, 131089, 1, 393227, 1, 1280527431, 1685353262, 808793134, 0, 196622, 0, 1, 524303, 4, 4, 1852399981, 0, 13, 17, 21, 196624, 4, 7, 262215, 13, 34, 0, 262215, 13, 33, 0, 262215, 17, 30, 0, 262215, 21, 30, 0, 131091, 2, 196641, 3, 2, 196630, 6, 32, 262167, 7, 6, 4, 589849, 10, 6, 1, 0, 0, 0, 1, 0, 196635, 11, 10, 262176, 12, 0, 11, 262203, 12, 13, 0, 262167, 15, 6, 2, 262176, 16, 1, 15, 262203, 16, 17, 1, 262176, 20, 3, 7, 262203, 20, 21, 3, 327734, 2, 4, 0, 3, 131320, 5, 262205, 11, 14, 13, 262205, 15, 18, 17, 327767, 7, 19, 14, 18, 196670, 21, 19, 65789, 65592
};

static uint32_t s_Depth_FragmentShaderBytecode[] =
{
    119734787, 66816, 851978, 24, 0, 131089, 1, 393227, 1, 1280527431, 1685353262, 808793134, 0, 196622, 0, 1, 524303, 4, 4, 1852399981, 0, 13, 17, 21, 196624, 4, 7, 262215, 13, 34, 0, 262215, 13, 33, 0, 262215, 17, 30, 0, 262215, 21, 30, 0, 131091, 2, 196641, 3, 2, 196630, 6, 32, 262167, 7, 6, 4, 589849, 10, 6, 1, 0, 0, 0, 1, 0, 196635, 11, 10, 262176, 12, 0, 11, 262203, 12, 13, 0, 262167, 15, 6, 2, 262176, 16, 1, 15, 262203, 16, 17, 1, 262176, 20, 3, 7, 262203, 20, 21, 3, 327734, 2, 4, 0, 3, 131320, 5, 262205, 11, 14, 13, 262205, 15, 18, 17, 327767, 7, 19, 14, 18, 589903, 7, 23, 19, 19, 0, 0, 0, 0, 196670, 21, 23, 65789, 65592
};

namespace vk
{

    GraphicsSwapchain GraphicsSwapchain::Create(VkInstance Instance, VkAllocationCallbacks *allocation_callback, VkPhysicalDevice PhysicalDevice, VkDevice Device, VkQueue Queue, uint32_t QueueFamilyIndex, VkFormat Format, PlatformWindow *Window, int BackBufferCount, int SyncInterval, _FrameInformation** pOutFrameInfo, bool UsingImGui)
    {
        GraphicsSwapchain gfxswap;
        gfxswap.m_Instance = Instance;
        gfxswap.m_allocation_callback = allocation_callback;
        gfxswap.m_Device = Device;
        gfxswap.m_Queue = Queue;
        gfxswap.m_PhysicalDevice = PhysicalDevice;
        gfxswap.m_QueueFamilyIndex = QueueFamilyIndex;
        gfxswap.m_BackBufferCount = BackBufferCount;
        gfxswap.m_Format = Format;
        gfxswap.m_SyncInterval = SyncInterval;
        gfxswap.m_Window = Window;
        gfxswap.m_SwapchainImageIndex = 0;
        gfxswap.m_LastFrameIndex = 0;
        gfxswap.m_UsingImGui = UsingImGui;
        assert(Device && PhysicalDevice && Queue && Instance && BackBufferCount > 0 && SyncInterval >= 0 && SyncInterval <= 4 && Window);
        /* Create Window Surface */
/*
#ifdef _WIN32
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        surfaceCreateInfo.hinstance = GetModuleHandleA(NULL);
        surfaceCreateInfo.hwnd = (HWND)Window->window_handle;
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(Instance, "vkCreateWin32SurfaceKHR");
        if (!vkCreateWin32SurfaceKHR)
        {
            log_error("Could not load vkCreateWin32SurfaceKHR function address. Maybe missing instance layer extension? or driver not updated.", __FILE__, __LINE__);
            Utils::Break();
        }
        vkCreateWin32SurfaceKHR(Instance, &surfaceCreateInfo, allocation_callback, &gfxswap.m_WindowSurface);
#else
        // Don't need to cast to GLWindow* since it is the first element.
        VkXlibSurfaceCreateInfoKHR surfaceCreateInfo{VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
        surfaceCreateInfo.dpy = *(Display**)Window->window_handle;
        vkCreateXlibSurfaceKHR(Instance, &surfaceCreateInfo, allocation_callback, &gfxswap.m_WindowSurface);
#endif
*/
        if(glfwCreateWindowSurface(Instance, Window->GetWindow(), allocation_callback, &gfxswap.m_WindowSurface) != VK_SUCCESS) {
            logerror("Failed to create window surface in swapchian.");
        }
        /* Create Window Surface */

        // Create Render Pass
        VkAttachmentDescription ColorAttachment{};
        ColorAttachment.format = gfxswap.m_Format;
        ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference ColorReference;
        ColorReference.attachment = 0;
        ColorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription ColorSubpass{};
        ColorSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        ColorSubpass.colorAttachmentCount = 1;
        ColorSubpass.pColorAttachments = &ColorReference;
        VkRenderPassCreateInfo RenderPassCreateInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        RenderPassCreateInfo.attachmentCount = 1;
        RenderPassCreateInfo.pAttachments = &ColorAttachment;
        RenderPassCreateInfo.subpassCount = 1;
        RenderPassCreateInfo.pSubpasses = &ColorSubpass;
        if (vkCreateRenderPass(gfxswap.m_Device, &RenderPassCreateInfo, allocation_callback, &gfxswap.m_PresentPass) != VK_SUCCESS)
        {
            log_error("Could not create present render pass for swapchain! fatal error.", __FILE__, __LINE__);
            Utils::Break();
        }

        VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.minLod = 1000.0f;
        samplerCreateInfo.maxLod = 1000.0f;
        samplerCreateInfo.maxAnisotropy = 16.0;
        vkCreateSampler(Device, &samplerCreateInfo, allocation_callback, &gfxswap.m_ColorTextureSampler);
        
        gfxswap.m_Swapchain = nullptr;
        gfxswap.CreateSwapchain(allocation_callback);

        // 1) compile shader source code into spirv
        uint32_t *pVSBytecode, *pFSBytecode;
        uint32_t VSBytecodeSize, FSBytecodeSize;
        if(ss_Compile_SourceCode) {
            logalert("Compiling (Vulkan) Swapchain Shader Pass.");
            Shader::CompileVulkanSPIRVText(s_VertexShaderSource, "VertexShaderSource.vert", shaderc_glsl_vertex_shader, (uint32_t **)&pVSBytecode, &VSBytecodeSize);
            Shader::CompileVulkanSPIRVText(s_FragmentShaderSource, "FragmentShaderSource.frag", shaderc_glsl_fragment_shader, (uint32_t **)&pFSBytecode, &FSBytecodeSize);
            FILE* temp__vs = fopen("swapchain_vertexbytecode.txt", "w");
            FILE* temp__fs = fopen("swapchain_fragmentbytecode.txt", "w");
            for(uint32_t i = 0; i < VSBytecodeSize / 4; i++) {
                std::string output = std::to_string(pVSBytecode[i]) + ", ";
                fwrite(output.c_str(), 1, output.size(), temp__vs);
            }
            for(uint32_t i = 0; i < FSBytecodeSize / 4; i++) {
                std::string output = std::to_string(pFSBytecode[i]) + ", ";
                fwrite(output.c_str(), 1, output.size(), temp__fs);
            }
            fclose(temp__vs);
            fclose(temp__fs);
        } else {
            loginfo("Loading Cached (Vulkan) Swapchain Shader Pass.");
            pVSBytecode = s_VertexShaderBytecode, VSBytecodeSize = sizeof(s_VertexShaderBytecode);
            pFSBytecode = s_FragmentShaderBytecode, FSBytecodeSize = sizeof(s_FragmentShaderBytecode);
        }

        // 2) create pipeline
        VkShaderModuleCreateInfo ModuleCreateInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ModuleCreateInfo.pCode = pVSBytecode;
        ModuleCreateInfo.codeSize = VSBytecodeSize;
        vkCreateShaderModule(Device, &ModuleCreateInfo, allocation_callback, &gfxswap.m_VertexModule);
        ModuleCreateInfo.pCode = pFSBytecode;
        ModuleCreateInfo.codeSize = FSBytecodeSize;
        vkCreateShaderModule(Device, &ModuleCreateInfo, allocation_callback, &gfxswap.m_FragmentModule);

        VkDescriptorSetLayoutBinding SetLayoutBinding{};
        SetLayoutBinding.binding = 0;
        SetLayoutBinding.descriptorCount = 1;
        SetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        SetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo SetLayoutCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        SetLayoutCreateInfo.bindingCount = 1;
        SetLayoutCreateInfo.pBindings = &SetLayoutBinding;
        vkCreateDescriptorSetLayout(Device, &SetLayoutCreateInfo, allocation_callback, &gfxswap.m_SetLayout);

        VkPipelineLayoutCreateInfo LayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        LayoutCreateInfo.setLayoutCount = 1;
        LayoutCreateInfo.pSetLayouts = &gfxswap.m_SetLayout;
        vkCreatePipelineLayout(Device, &LayoutCreateInfo, allocation_callback, &gfxswap.m_Layout);

        // create descriptor pool
        std::array<VkDescriptorPoolSize, 2> PoolSizes;
        PoolSizes[0].descriptorCount = BackBufferCount;
        PoolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        PoolSizes[1].descriptorCount = BackBufferCount;
        PoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        VkDescriptorPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolCreateInfo.maxSets = BackBufferCount + 1;
        poolCreateInfo.poolSizeCount = PoolSizes.size();
        poolCreateInfo.pPoolSizes = PoolSizes.data();
        vkCreateDescriptorPool(Device, &poolCreateInfo, allocation_callback, &gfxswap.m_DescriptorPool);
        // create descriptor set
        gfxswap.m_DescriptorSets = new VkDescriptorSet[BackBufferCount];
        for (int i = 0; i < BackBufferCount; i++)
        {
            VkDescriptorSetAllocateInfo AllocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            AllocateInfo.descriptorPool = gfxswap.m_DescriptorPool;
            AllocateInfo.descriptorSetCount = 1;
            AllocateInfo.pSetLayouts = &gfxswap.m_SetLayout;
            vkcheck(vkAllocateDescriptorSets(Device, &AllocateInfo, &gfxswap.m_DescriptorSets[i]));
        }

        // create command objects
        VkCommandPoolCreateInfo CommandPoolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
        CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkcheck(vkCreateCommandPool(Device, &CommandPoolCreateInfo, allocation_callback, &gfxswap.m_CommandPool));

        gfxswap.m_CommandBuffers = new VkCommandBuffer[BackBufferCount];
        VkCommandBufferAllocateInfo AllocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        AllocateInfo.commandPool = gfxswap.m_CommandPool;
        AllocateInfo.commandBufferCount = BackBufferCount;
        AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vkcheck(vkAllocateCommandBuffers(Device, &AllocateInfo, gfxswap.m_CommandBuffers));

        gfxswap.CreatePipeline(allocation_callback);

        gfxswap.m_FrameInfo = new _FrameInformation();
        gfxswap.m_FrameInfo->m_FrameCount = BackBufferCount;
        gfxswap.m_FrameInfo->m_LastFrameIndex = 0;
        gfxswap.m_FrameInfo->m_FrameIndex = 0;
        gfxswap.m_FrameInfo->m_Initalized = true;
        if (pOutFrameInfo)
            *pOutFrameInfo = gfxswap.m_FrameInfo;

        // Frame Sync
        gfxswap.m_FrameCount = 0;
        // The is size of the following is equal to m_BackBufferCount
        gfxswap.m_FrameFences = new VkFence[BackBufferCount];
        gfxswap.m_FrameSemaphores = new GPUSemaphore(); //new VkSemaphore[BackBufferCount];
        gfxswap.m_FrameSemaphores->m_ApiType = 0;
        gfxswap.m_FrameSemaphores->m_context = nullptr;
        gfxswap.m_FrameSemaphores->m_FrameInfo = gfxswap.m_FrameInfo;
        gfxswap.m_FrameSemaphores->m_semaphoreptr = new VkSemaphore[BackBufferCount];
        gfxswap.m_FrameSwapchainCompleteSemaphores = new VkSemaphore[BackBufferCount];

        for (int i = 0; i < BackBufferCount; i++)
        {
            VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            vkcheck(vkCreateFence(Device, &fenceCreateInfo, allocation_callback, &gfxswap.m_FrameFences[i]));
            VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            vkcheck(vkCreateSemaphore(Device, &semaphoreCreateInfo, allocation_callback, &gfxswap.m_FrameSemaphores->m_semaphoreptr[i]));
            vkcheck(vkCreateSemaphore(Device, &semaphoreCreateInfo, allocation_callback, &gfxswap.m_FrameSwapchainCompleteSemaphores[i]));
        }

        // cleanup/free memory
        if (ss_Compile_SourceCode) {
            delete[] pVSBytecode;
            delete[] pFSBytecode;
        }

        return gfxswap;
    }

    void GraphicsSwapchain::CreateSwapchain(VkAllocationCallbacks *allocation_callback)
    {
        // Get information for SwapChain
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_WindowSurface, &surfaceCapabilities);
        uint32_t surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_WindowSurface, &surfaceFormatCount, NULL);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_WindowSurface, &surfaceFormatCount, surfaceFormats.data());
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_WindowSurface, &presentModeCount, NULL);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_WindowSurface, &presentModeCount, presentModes.data());

        if (m_BackBufferCount < surfaceCapabilities.minImageCount)
        {
            char buf[150];
            buf[0] = 0;
            strcat(buf, "Swapchain BackBufferCount is not enough for the graphics card. Setting BackBufferCount to default. ");
            char temp[15];
            temp[0] = 0;
            //itoa(surfaceCapabilities.minImageCount, temp, 10);
            strcat(buf, temp);
            logwarning(buf);
        }
        Utils::ClampValues(m_BackBufferCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);

        // Check SwapChain Surface Format is supported
        bool FormatSupported = false;
        VkColorSpaceKHR ColorSpace;
        for (const auto &sf : surfaceFormats)
        {
            if (sf.format == m_Format)
            {
                ColorSpace = sf.colorSpace;
                FormatSupported = true;
                break;
            }
        }

        if (!FormatSupported)
        {
            logalert("Surface format is not supported on this graphics card. Could not create swapchain. Defaulting to the first supported surface format.");
            if(surfaceFormats.size() > 0) {
                m_Format = surfaceFormats[0].format;
                ColorSpace = surfaceFormats[0].colorSpace;
            } else {
                logerror("No surface format found!");
                Utils::FatalBreak();
            }
        }

        // Choose PresentMode
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        if (m_SyncInterval > 0)
        {
            for (const auto &pm : presentModes)
            {
                if (pm == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
                {
                    presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                    break;
                }
                else if (pm == VK_PRESENT_MODE_FIFO_KHR)
                {
                    presentMode = VK_PRESENT_MODE_FIFO_KHR;
                    break;
                }
            }
        }

        // Choose Swap Extent
        VkExtent2D swapExtend2d;
        if (surfaceCapabilities.currentExtent.width == UINT_MAX)
        {
            swapExtend2d = surfaceCapabilities.currentExtent;
        }
        else
        {
            swapExtend2d.width = m_Window->GetWidth();
            swapExtend2d.height = m_Window->GetHeight();
        }
        Utils::ClampValues(swapExtend2d.width, surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.minImageExtent.width);
        Utils::ClampValues(swapExtend2d.height, surfaceCapabilities.maxImageExtent.height, surfaceCapabilities.minImageExtent.height);
        m_FramebufferWidth = swapExtend2d.width;
        m_FramebufferHeight = swapExtend2d.height;

        VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        createInfo.surface = m_WindowSurface;
        createInfo.minImageCount = m_BackBufferCount;
        createInfo.imageFormat = m_Format;
        createInfo.imageColorSpace = ColorSpace;
        createInfo.imageExtent = swapExtend2d;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.clipped = VK_TRUE;

        VkBool32 supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_QueueFamilyIndex, m_WindowSurface, &supported);
        if (supported == VK_TRUE)
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        else
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = m_Swapchain;

        VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, allocation_callback, &m_Swapchain);

        if (result != VK_SUCCESS)
        {
            log_error("Could not create Vulkan Swap chain!", __FILE__, __LINE__);
            Utils::Break();
        }

        if (!m_Framebuffers)
            m_Framebuffers = new VkFramebuffer[m_BackBufferCount];
        if (!m_Views)
            m_Views = new VkImageView[m_BackBufferCount];

        if (!m_SwapchainImages)
            m_SwapchainImages = new VkImage[m_BackBufferCount];
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, reinterpret_cast<uint32_t *>(&m_BackBufferCount), m_SwapchainImages);

        for (uint32_t i = 0; i < m_BackBufferCount; i++)
        {
            VkImageViewCreateInfo viewCreateInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            viewCreateInfo.image = m_SwapchainImages[i];
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = m_Format;
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = 1;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.layerCount = 1;
            VkImageView view;
            if (vkCreateImageView(m_Device, &viewCreateInfo, allocation_callback, &view) != VK_SUCCESS)
            {
                log_error("Could not create swapchain image view.", __FILE__, __LINE__);
                Utils::Break();
            }
            m_Views[i] = view;
            VkFramebufferCreateInfo fbCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fbCreateInfo.renderPass = m_PresentPass;
            fbCreateInfo.attachmentCount = 1;
            VkImageView framebufferAttachments[1] = {view};
            fbCreateInfo.pAttachments = framebufferAttachments;
            fbCreateInfo.width = swapExtend2d.width;
            fbCreateInfo.height = swapExtend2d.height;
            fbCreateInfo.layers = 1;
            VkFramebuffer framebuffer;
            vkcheck(vkCreateFramebuffer(m_Device, &fbCreateInfo, allocation_callback, &framebuffer));
            m_Framebuffers[i] = framebuffer;
        }

        // Transistion Swapchain images into VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to stop validation errors
        VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        VkCommandPool pool;
        vkCreateCommandPool(m_Device, &poolCreateInfo, nullptr, &pool);
        VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        for (uint32_t i = 0; i < m_BackBufferCount; i++)
        {
            VkImageMemoryBarrier barrier;
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = 0;
            barrier.dstQueueFamilyIndex = 0;
            barrier.image = m_SwapchainImages[i];
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        vkQueueSubmit(m_Queue, 1, &submitInfo, nullptr);
        vkQueueWaitIdle(m_Queue);
        vkDestroyCommandPool(m_Device, pool, nullptr);

        // initalize ImGui
        if (m_UsingImGui && !m_GuiInitalized)
        {
            Gui_VKInitalizeImGui(m_Instance, allocation_callback, m_PhysicalDevice, m_QueueFamilyIndex, m_BackBufferCount, m_Device, m_PresentPass, m_Window);
            m_GuiInitalized = true;
        }
    }

    void GraphicsSwapchain::CreatePipeline(VkAllocationCallbacks *allocation_callback)
    {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

        VkPipelineShaderStageCreateInfo ShaderStages[2]{};
        ShaderStages[0].sType = ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[0].pName = "main";
        ShaderStages[0].module = m_VertexModule;
        ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStages[1].pName = "main";
        ShaderStages[1].module = m_FragmentModule;
        ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = ShaderStages;

        VkPipelineVertexInputStateCreateInfo VertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VertexInputState.vertexBindingDescriptionCount = 0;
        VertexInputState.pVertexBindingDescriptions = NULL;
        VertexInputState.vertexAttributeDescriptionCount = 0;
        VertexInputState.pVertexAttributeDescriptions = NULL;
        pipelineCreateInfo.pVertexInputState = &VertexInputState;

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipelineCreateInfo.pInputAssemblyState = &InputAssemblyState;

        VkViewport viewport;
        viewport.x = viewport.y = 0;
        viewport.width = m_Window->GetWidth();
        viewport.height = m_Window->GetHeight();
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor;
        scissor.offset.x = scissor.offset.y = 0;
        scissor.extent.width = m_Window->GetWidth();
        scissor.extent.height = m_Window->GetHeight();

        VkPipelineViewportStateCreateInfo ViewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        ViewportState.viewportCount = 1;
        ViewportState.scissorCount = 1;
        ViewportState.pViewports = &viewport;
        ViewportState.pScissors = &scissor;
        pipelineCreateInfo.pViewportState = &ViewportState;

        VkPipelineRasterizationStateCreateInfo RasterizeState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        RasterizeState.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizeState.cullMode = VK_CULL_MODE_NONE;
        RasterizeState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        RasterizeState.lineWidth = 1.0f;
        pipelineCreateInfo.pRasterizationState = &RasterizeState;

        VkPipelineMultisampleStateCreateInfo MultisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        MultisampleState.minSampleShading = 1.0f;
        pipelineCreateInfo.pMultisampleState = &MultisampleState;

        VkPipelineDepthStencilStateCreateInfo DepthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        DepthStencilState.depthTestEnable = VK_FALSE;
        DepthStencilState.depthWriteEnable = VK_TRUE;
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
        pipelineCreateInfo.pDepthStencilState = &DepthStencilState;

        VkPipelineColorBlendAttachmentState ColorBlend0{};
        ColorBlend0.blendEnable = VK_FALSE;
        ColorBlend0.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo ColorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        ColorBlendState.attachmentCount = 1;
        ColorBlendState.pAttachments = &ColorBlend0;
        pipelineCreateInfo.pColorBlendState = &ColorBlendState;

        pipelineCreateInfo.layout = m_Layout;
        pipelineCreateInfo.renderPass = m_PresentPass;

        VkGraphicsPipelineCreateInfo pipelineDepthCreateInfo = pipelineCreateInfo;

        VkShaderModuleCreateInfo depthModuleCreateInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        depthModuleCreateInfo.codeSize = sizeof(s_Depth_FragmentShaderBytecode);
        depthModuleCreateInfo.pCode = s_Depth_FragmentShaderBytecode;
        VkShaderModule FragmentDepthModule;
        vkCreateShaderModule(m_Device, &depthModuleCreateInfo, nullptr, &FragmentDepthModule);

        VkPipelineShaderStageCreateInfo ShaderDepthStages[2]{};
        ShaderDepthStages[0].sType = ShaderDepthStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderDepthStages[0].pName = "main";
        ShaderDepthStages[0].module = m_VertexModule;
        ShaderDepthStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderDepthStages[1].pName = "main";
        ShaderDepthStages[1].module = FragmentDepthModule;
        ShaderDepthStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineDepthCreateInfo.pStages = ShaderDepthStages;
        
        vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, allocation_callback, &m_PresentPipeline);
        vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineDepthCreateInfo, allocation_callback, &m_PresentDepthPipeline);
        vkDestroyShaderModule(m_Device, FragmentDepthModule, nullptr);

    }

    void GraphicsSwapchain::PrepareNextFrame(uint32_t *pNextImageIndex, IGPUSemaphore *pSwapchainReadySemaphore)
    {
        PROFILE_FUNCTION();
        assert(pNextImageIndex && pSwapchainReadySemaphore);
        uint32_t FrameIndex = m_FrameCount % m_BackBufferCount;
        uint32_t ImageIndex;
        VkResult result;
        result = vkAcquireNextImageKHR(m_Device, m_Swapchain, 5000'000'000, m_FrameSemaphores->m_semaphoreptr[FrameIndex], nullptr/*m_FrameFences[FrameIndex]*/, &ImageIndex);
        if (result != VK_SUCCESS)
        {
            logwarning("vkAcquireNextImageKHR(...) failed, it either timed out or there was an error or the swapchain was resized! (Max Time for next frame to be ready is 5 seconds)");
        }
        if (m_UsingImGui)
            Gui_VKBeginGUIFrame();
        m_SwapchainImageIndex = ImageIndex;
        *pNextImageIndex = FrameIndex;
        *pSwapchainReadySemaphore = m_FrameSemaphores;
    }

    void GraphicsSwapchain::Present(VkImage ColorTexture, VkImageView ColorTextureView, VkImageLayout ImageLayout, uint32_t WaitSemaphoreCount, VkSemaphore* pWaitSemaphores, bool DepthPipeline)
    {
        PROFILE_FUNCTION();
        uint32_t FrameIndex = m_FrameCount % m_BackBufferCount;
        m_LastFrameIndex = FrameIndex;

        {
            PROFILE_SCOPE("[Vulkan] Swapchain Present Pass");
            VkCommandBufferBeginInfo CmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            CmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkResetCommandBuffer(m_CommandBuffers[FrameIndex], 0);
            vkBeginCommandBuffer(m_CommandBuffers[FrameIndex], &CmdBeginInfo);

            VkRenderPassBeginInfo PassBeginInfo;
            PassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            PassBeginInfo.pNext = NULL;
            PassBeginInfo.renderPass = m_PresentPass;
            PassBeginInfo.framebuffer = m_Framebuffers[FrameIndex];
            PassBeginInfo.renderArea.offset = {0, 0};
            PassBeginInfo.renderArea.extent = {m_FramebufferWidth, m_FramebufferHeight};
            PassBeginInfo.clearValueCount = 0;
            PassBeginInfo.pClearValues = NULL;

            // Set Image to DescriptorSet
            VkDescriptorImageInfo ImageWriteInfo;
            ImageWriteInfo.sampler = m_ColorTextureSampler;
            ImageWriteInfo.imageView = ColorTextureView;
            ImageWriteInfo.imageLayout = ImageLayout;
            VkWriteDescriptorSet DescriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            DescriptorWrite.dstSet = m_DescriptorSets[FrameIndex];
            DescriptorWrite.descriptorCount = 1;
            DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            DescriptorWrite.pImageInfo = &ImageWriteInfo;
            // TODO: Avoid doing this every frame.
            {
                PROFILE_SCOPE("[Vulkan] Updating Presenting Descriptor Set");
                vkUpdateDescriptorSets(m_Device, 1, &DescriptorWrite, 0, NULL);
            }

            vkCmdBeginRenderPass(m_CommandBuffers[FrameIndex], &PassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(m_CommandBuffers[FrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, DepthPipeline ? m_PresentDepthPipeline : m_PresentPipeline);
            vkCmdBindDescriptorSets(m_CommandBuffers[FrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, 0, 1, &m_DescriptorSets[FrameIndex], 0, NULL);
            vkCmdDraw(m_CommandBuffers[FrameIndex], 6, 1, 0, 0);
        }

        {
            PROFILE_SCOPE("[Vulkan] Swapchain ImGui Recording.");
            if (m_UsingImGui)
                Gui_VKEndGUIFrame(m_CommandBuffers[FrameIndex]);
        }

        vkCmdEndRenderPass(m_CommandBuffers[FrameIndex]);

        vkEndCommandBuffer(m_CommandBuffers[FrameIndex]);

        VkPipelineStageFlags pWaitDstStageMask[60];
        for(uint32_t i = 0; i < WaitSemaphoreCount; i++)
            pWaitDstStageMask[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        
        VkSubmitInfo SubmitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        SubmitInfo.waitSemaphoreCount = WaitSemaphoreCount;
        SubmitInfo.pWaitSemaphores = pWaitSemaphores;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &m_CommandBuffers[FrameIndex];
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = &m_FrameSwapchainCompleteSemaphores[FrameIndex];
        SubmitInfo.pWaitDstStageMask = pWaitDstStageMask;

        vkQueueSubmit(m_Queue, 1, &SubmitInfo, 0);

        VkPresentInfoKHR PresentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores = &m_FrameSwapchainCompleteSemaphores[FrameIndex];
        PresentInfo.swapchainCount = 1;
        PresentInfo.pSwapchains = &m_Swapchain;
        PresentInfo.pImageIndices = &m_SwapchainImageIndex;
        
        {
            PROFILE_SCOPE("[Vulkan] Swapchain vkQueuePresentKHR");
            VkResult present_status = vkQueuePresentKHR(m_Queue, &PresentInfo);
            if (present_status != VK_SUCCESS)
            {
                // Did the window minimize?
                if (m_Window->IsWindowMinimized())
                    goto NoResize;
                // Wait until swapchain is not used in-flight
                vkQueueWaitIdle(m_Queue); // TODO: Fix validation layer errors when resizing
                // the swapchain is to old, must be recreated.
                // Before creating a new swapchain destroy views and framebuffers
                for (uint32_t i = 0; i < m_BackBufferCount; i++)
                {
                    vkDestroyImageView(m_Device, m_Views[i], m_allocation_callback);
                    vkDestroyFramebuffer(m_Device, m_Framebuffers[i], m_allocation_callback);
                }
                CreateSwapchain(m_allocation_callback);
                // Recreate Pipeline since viewport/scissor have changed
                vkDestroyPipeline(m_Device, m_PresentPipeline, m_allocation_callback);
                CreatePipeline(m_allocation_callback);
            }
        }
        
        NoResize:
        m_FrameCount++;
    }

    void GraphicsSwapchain::Destroy()
    {
        PROFILE_FUNCTION();
        vkQueueWaitIdle(m_Queue); // make sure swapchain is not in-flight
        vkDestroyShaderModule(m_Device, m_VertexModule, m_allocation_callback);
        vkDestroyShaderModule(m_Device, m_FragmentModule, m_allocation_callback);
        vkDestroyPipeline(m_Device, m_PresentPipeline, m_allocation_callback);
        vkDestroyRenderPass(m_Device, m_PresentPass, m_allocation_callback);
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, m_allocation_callback);
        vkDestroyDescriptorSetLayout(m_Device, m_SetLayout, m_allocation_callback);
        delete[] m_DescriptorSets;
        vkDestroyPipelineLayout(m_Device, m_Layout, m_allocation_callback);
        delete[] m_SwapchainImages;
        for (uint32_t i = 0; i < m_BackBufferCount; i++)
        {
            vkDestroyImageView(m_Device, m_Views[i], m_allocation_callback);
            vkDestroyFramebuffer(m_Device, m_Framebuffers[i], m_allocation_callback);
            vkDestroyFence(m_Device, m_FrameFences[i], m_allocation_callback);
            vkDestroySemaphore(m_Device, m_FrameSemaphores->m_semaphoreptr[i], m_allocation_callback);
            vkDestroySemaphore(m_Device, m_FrameSwapchainCompleteSemaphores[i], m_allocation_callback);
        }
        delete m_FrameSemaphores->m_semaphoreptr;
        delete m_FrameSemaphores;
        delete m_FrameFences;
        delete m_FrameSwapchainCompleteSemaphores;
        vkDestroySwapchainKHR(m_Device, m_Swapchain, m_allocation_callback);
        vkDestroySurfaceKHR(m_Instance, m_WindowSurface, m_allocation_callback);
        vkDestroySampler(m_Device, m_ColorTextureSampler, m_allocation_callback);
        if (m_UsingImGui)
            Gui_VKDestroyImGui(m_allocation_callback);
        delete[] m_Framebuffers;
        delete[] m_Views;
        vkDestroyCommandPool(m_Device, m_CommandPool, m_allocation_callback);
        delete[] m_CommandBuffers;
        delete m_FrameInfo;
    }

}