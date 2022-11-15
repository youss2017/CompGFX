#include "egxswapchain.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <algorithm>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <Utility/CppUtility.hpp>
#include "../cmd/cmd.hpp"
#include "../core/shaders/egxshader.hpp"

static const char *builtin_vertex_shader = R"(
	#version 450

	const vec4 verts[6] =
	{
		vec4(-1.0, -1.0, 0.0, 0.0),
		vec4(-1.0, 1.0, 0.0, 1.0),
		vec4(1.0, -1.0, 1.0, 0.0),
		vec4(-1.0, 1.0, 0.0, 1.0),
		vec4(1.0, 1.0, 1.0, 1.0),
		vec4(1.0, -1.0, 1.0, 0.0)
	};

	layout (location = 0) out vec2 UV;

	void main() {
		vec4 v = verts[gl_VertexIndex];
		UV = v.zw;
		gl_Position = vec4(v.xy, 0.0, 1.0);
	}
)";

static const char *builtin_fragment_shader = R"(
	#version 450

	layout (binding = 0) uniform sampler2D BlitImage; 
	layout (location = 0) in vec2 UV;
	layout (location = 0) out vec4 FragColor;

	void main()
	{
		FragColor = texture(BlitImage, UV);
	}
)";

using namespace egx;

egx::VulkanSwapchain::VulkanSwapchain(ref<VulkanCoreInterface> &CoreInterface, void *GlfwWindowPtr, bool VSync, bool SetupImGui, bool ClearSwapchain)
	: _core(CoreInterface), GlfwWindowPtr(GlfwWindowPtr), _vsync(VSync), _imgui(SetupImGui), _clearswapchain(ClearSwapchain)
{
	VkBool32 Supported{};
	glfwCreateWindowSurface(_core->Instance, (GLFWwindow *)GlfwWindowPtr, nullptr, &Surface);
	vkGetPhysicalDeviceSurfaceSupportKHR(CoreInterface->PhysicalDevice.Id, CoreInterface->QueueFamilyIndex, Surface, &Supported);
	if (!Supported)
	{
		vkDestroySurfaceKHR(_core->Instance, Surface, nullptr);
		throw std::runtime_error("The provided window does not support swapchain presentation. Look up vkGetPhysicalDeviceSurfaceSupportKHR()");
	}
	uint32_t surfaceCount, presentationModeCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_core->PhysicalDevice.Id, Surface, &surfaceCount, nullptr);
	_surfaceFormats.resize(surfaceCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(_core->PhysicalDevice.Id, Surface, &surfaceCount, _surfaceFormats.data());
	vkGetPhysicalDeviceSurfacePresentModesKHR(_core->PhysicalDevice.Id, Surface, &presentationModeCount, nullptr);
	_presentationModes.resize(presentationModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(_core->PhysicalDevice.Id, Surface, &presentationModeCount, _presentationModes.data());

	glfwGetFramebufferSize((GLFWwindow *)GlfwWindowPtr, &_width, &_height);

	_pool = CreateCommandPool(_core, true, false, true, false);
	_cmd = _pool->AllocateBufferFrameFlightMode(true);
	_swapchain_acquire_lock = Fence::FactoryCreate(_core, true);
	_cmd_lock = Fence::FactoryCreate(_core, true);

	_present_lock = Semaphore::FactoryCreate(CoreInterface, "PresentLock");
	_blit_lock = Semaphore::FactoryCreate(CoreInterface, "InternalBlitLock");
	Synchronization.SetConsumerWaitSemaphore(_present_lock);

	if (_imgui)
	{
#if defined(VK_NO_PROTOTYPES)
		struct ImGUI_LoadFunctions_UserData
		{
			VkInstance instance;
			VkDevice device;
		};
		ImGUI_LoadFunctions_UserData ImGui_Load_UserData;
		ImGui_Load_UserData.instance = CoreInterface->Instance;
		ImGui_Load_UserData.device = CoreInterface->Device;
		ImGui_ImplVulkan_LoadFunctions(
			[](const char *function_name, void *user_data) throw()->PFN_vkVoidFunction {
				ImGUI_LoadFunctions_UserData *pUserData = (ImGUI_LoadFunctions_UserData *)user_data;
				PFN_vkVoidFunction proc = (PFN_vkVoidFunction)vkGetDeviceProcAddr(pUserData->device, function_name);
				if (!proc)
					return vkGetInstanceProcAddr(pUserData->instance, function_name);
				else
					return proc;
			},
			&ImGui_Load_UserData);
#endif

		_descriptor_set_pool = SetPool::FactoryCreate(CoreInterface);
		SetPoolRequirementsInfo req;
		req.SetCount = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_SAMPLER] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC] = 1000;
		req.Type[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT] = 1000;
		_descriptor_set_pool->AdjustForRequirements(req);

		CreateRenderPass();
		CreateSwapchain(_width, _height);
		CreatePipelineObjects();
		CreatePipeline();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGuiIO &io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
		// io.ConfigDockingWithShift = true;

		VkQueue queue;
		vkGetDeviceQueue(CoreInterface->Device, CoreInterface->QueueFamilyIndex, 0, &queue);
		ImGui_ImplGlfw_InitForVulkan((GLFWwindow *)this->GlfwWindowPtr, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = CoreInterface->Instance;
		init_info.PhysicalDevice = CoreInterface->PhysicalDevice.Id;
		init_info.Device = CoreInterface->Device;
		init_info.QueueFamily = CoreInterface->QueueFamilyIndex;
		init_info.Queue = queue;
		init_info.PipelineCache = NULL;
		init_info.DescriptorPool = _descriptor_set_pool->GetSetPool();
		init_info.Allocator = NULL;
		init_info.MinImageCount = this->_core->MaxFramesInFlight;
		init_info.ImageCount = this->_core->MaxFramesInFlight;
		init_info.CheckVkResultFn = NULL;
		ImGui_ImplVulkan_Init(&init_info, RenderPass);
		// ImFont* fftt = io.Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCodePL-SemiBold.ttf", 14.5f);

		// Upload Fonts
		{
			// Use any command queue
			VkCommandPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
			VkCommandPool command_pool;
			vkCreateCommandPool(CoreInterface->Device, &createInfo, nullptr, &command_pool);
			VkCommandBuffer command_buffer;
			VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
			allocInfo.commandPool = command_pool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			vkAllocateCommandBuffers(CoreInterface->Device, &allocInfo, &command_buffer);

			vkResetCommandPool(CoreInterface->Device, command_pool, 0);
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
			vkDestroyCommandPool(CoreInterface->Device, command_pool, nullptr);
		}
	}
}

egx::VulkanSwapchain::VulkanSwapchain(VulkanSwapchain &&move) noexcept
{
	memcpy(this, &move, sizeof(VulkanSwapchain));
	memset(&move, 0, sizeof(VulkanCoreInterface));
}

egx::VulkanSwapchain &egx::VulkanSwapchain::operator=(VulkanSwapchain &move) noexcept
{
	this->~VulkanSwapchain();
	memcpy(this, &move, sizeof(VulkanSwapchain));
	memset(&move, 0, sizeof(VulkanCoreInterface));
	return *this;
}

egx::VulkanSwapchain::~VulkanSwapchain()
{
	if (_imgui)
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	if (RenderPass)
		vkDestroyRenderPass(_core->Device, RenderPass, nullptr);
	if (Swapchain)
		vkDestroySwapchainKHR(_core->Device, Swapchain, nullptr);
	if (Surface)
		vkDestroySurfaceKHR(_core->Instance, Surface, nullptr);
	for (auto view : Views)
		vkDestroyImageView(_core->Device, view, nullptr);
	for (auto fbo : _framebuffers)
		vkDestroyFramebuffer(_core->Device, fbo, nullptr);
	_cmd_lock->SynchronizeAllFrames();
	_swapchain_acquire_lock->SynchronizeAllFrames();
}

void egx::VulkanSwapchain::Acquire()
{
	auto fence = _swapchain_acquire_lock->GetFence();
	vkWaitForFences(_core->Device, 1, &fence, VK_TRUE, UINT64_MAX);
	vkResetFences(_core->Device, 1, &fence);
	VkResult result = vkAcquireNextImageKHR(_core->Device,
											Swapchain,
											UINT64_MAX,
											_present_lock->GetSemaphore(),
											fence,
											&_core->CurrentFrame);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		Resize();
	if (_imgui)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
}

uint32_t egx::VulkanSwapchain::PresentInit()
{
	auto poollock = _cmd_lock->GetFence();
	vkWaitForFences(_core->Device, 1, &poollock, VK_TRUE, UINT64_MAX);
	vkResetFences(_core->Device, 1, &poollock);
	return _core->CurrentFrame;
}

void egx::VulkanSwapchain::Present(const ref<Image> &image, uint32_t viewIndex)
{
	uint32_t frame = PresentInit();

	if (image->Img != _last_updated_image[frame])
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = _image_sampler->GetSampler();
		imageInfo.imageView = image->view(viewIndex);
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		write.dstSet = _descriptor_set[frame];
		write.dstBinding = 0;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(_core->Device, 1, &write, 0, nullptr);
		_last_updated_image[frame] = image->Img;
	}

	CommandBuffer::StartCommandBuffer(_cmd[frame], 0);
	DInsertDebugLabel(_core, _cmd[frame], frame, "Swapchain Present", 0.5f, 0.4f, 0.65f);

	VkClearValue clear{};
	VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	beginInfo.renderPass = RenderPass;
	beginInfo.framebuffer = _framebuffers[frame];
	beginInfo.renderArea = {{}, {(uint32_t)_width, (uint32_t)_height}};
	beginInfo.clearValueCount = _clearswapchain ? 1 : 0;
	beginInfo.pClearValues = &clear;
	vkCmdBeginRenderPass(_cmd[frame], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(_cmd[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, _blit_gfx);
	vkCmdBindDescriptorSets(_cmd[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, _blit_layout, 0, 1, &_descriptor_set[frame], 0, nullptr);
	vkCmdDraw(_cmd[frame], 6, 1, 0, 0);
	if (_imgui)
	{
		ImGui::Render();
		auto draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, _cmd[frame]);
	}
	vkCmdEndRenderPass(_cmd[frame]);

	vkEndCommandBuffer(_cmd[frame]);

	DEndDebugLabel(_core, _cmd[frame]);

	PresentCommon(frame, false);
}

void egx::VulkanSwapchain::Present()
{
	assert(_imgui && "To use default Present() you must have enabled imgui otherwise you are presenting nothing.");
	uint32_t frame = PresentInit();

	CommandBuffer::StartCommandBuffer(_cmd[frame], 0);
	VkClearValue clear{};
	VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	beginInfo.renderPass = RenderPass;
	beginInfo.framebuffer = _framebuffers[frame];
	beginInfo.renderArea = {{}, {(uint32_t)_width, (uint32_t)_height}};
	beginInfo.clearValueCount = _clearswapchain ? 1 : 0;
	beginInfo.pClearValues = &clear;
	vkCmdBeginRenderPass(_cmd[frame], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	if (_imgui)
	{
		ImGui::Render();
		auto draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, _cmd[frame]);
	}
	vkCmdEndRenderPass(_cmd[frame]);
	vkEndCommandBuffer(_cmd[frame]);

	PresentCommon(frame, true);
}

void egx::VulkanSwapchain::PresentCommon(uint32_t frame, bool onlyimgui)
{
	if (_imgui)
	{
		auto &io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			auto backup_context = ImGui::GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			ImGui::SetCurrentContext(backup_context);
		}
	}

	VkSemaphore blitLock = _blit_lock->GetSemaphore();
	auto waitSemaphores = Synchronization.GetProducerWaitSemaphores(frame);
	auto waitDstStages = Synchronization.GetProducerWaitStageFlags();

	if (onlyimgui)
	{
		waitSemaphores.push_back(_present_lock->GetSemaphore());
		waitDstStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	CommandBuffer::Submit(_core, {_cmd[frame]},
						  waitSemaphores,
						  waitDstStages,
						  {blitLock},
						  _cmd_lock->GetFence());

	VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &blitLock;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &Swapchain;
	presentInfo.pImageIndices = &frame;
	vkQueuePresentKHR(_core->Queue, &presentInfo);
}

void egx::VulkanSwapchain::SetSyncInterval(bool VSync)
{
	_vsync = VSync;
	CreateSwapchain(_width, _height);
}

void egx::VulkanSwapchain::Resize(int width, int height)
{
	assert(width > 0 && height > 0);
	CreateSwapchain(_width, _height);
	CreatePipeline();
}

void egx::VulkanSwapchain::Resize()
{
	vkDeviceWaitIdle(_core->Device);
	int w, h;
	glfwGetFramebufferSize((GLFWwindow *)GlfwWindowPtr, &w, &h);
	if (w == 0 || h == 0)
		return;
	_width = w, _height = h;
	CreateSwapchain(_width, _height);
}

VkSurfaceFormatKHR egx::VulkanSwapchain::GetSurfaceFormat()
{
	std::vector<VkFormat> desiredList = {
		{VK_FORMAT_B8G8R8A8_UNORM},
		{VK_FORMAT_R8G8B8A8_UNORM},
		{VK_FORMAT_B8G8R8A8_UINT},
		{VK_FORMAT_R8G8B8A8_UINT},
		{VK_FORMAT_B8G8R8A8_SINT},
		{VK_FORMAT_R8G8B8A8_SINT},
		{VK_FORMAT_B8G8R8_UNORM},
		{VK_FORMAT_R8G8B8_UNORM},
		{VK_FORMAT_B8G8R8A8_UINT},
		{VK_FORMAT_R8G8B8A8_UINT},
		{VK_FORMAT_B8G8R8_UINT},
		{VK_FORMAT_R8G8B8_UINT},
		{VK_FORMAT_B8G8R8_SINT},
		{VK_FORMAT_R8G8B8_SINT},
		{VK_FORMAT_R8G8B8_UNORM}};
	std::vector<VkFormat> supportedList;
	for (auto &surfaceFormat : _surfaceFormats)
	{
		supportedList.push_back(surfaceFormat.format);
	}
	for (int i = 0; i < desiredList.size(); i++)
	{
		auto temp = std::find(supportedList.begin(), supportedList.end(), desiredList[i]);
		if (temp != supportedList.end())
			return _surfaceFormats[temp - supportedList.begin()];
	}
	return _surfaceFormats[0];
}

VkPresentModeKHR egx::VulkanSwapchain::GetPresentMode()
{
	if (!_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_MAILBOX_KHR))
		return VK_PRESENT_MODE_MAILBOX_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR))
		return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_FIFO_KHR))
		return VK_PRESENT_MODE_FIFO_KHR;
	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

void egx::VulkanSwapchain::CreateSwapchain(int width, int height)
{
	vkQueueWaitIdle(_core->Queue);
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_core->PhysicalDevice.Id, Surface, &_capabilities);
	VkSurfaceFormatKHR surfaceFormat = GetSurfaceFormat();
	_width = width;
	_height = height;

	for (auto view : Views)
		vkDestroyImageView(_core->Device, view, nullptr);
	for (auto fbo : _framebuffers)
		vkDestroyFramebuffer(_core->Device, fbo, nullptr);

	VkExtent2D swapchainSize =
		{
			std::clamp<uint32_t>(width, _capabilities.minImageExtent.width, _capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, _capabilities.minImageExtent.height, _capabilities.maxImageExtent.height)};
	_width = swapchainSize.width;
	_height = swapchainSize.height;

	_core->MaxFramesInFlight = std::max<uint32_t>(_core->MaxFramesInFlight, _capabilities.minImageCount);
	assert(_core->MaxFramesInFlight <= 5);

	VkSwapchainKHR oldSwapchain = Swapchain;
	VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	createInfo.surface = Surface;
	createInfo.minImageCount = _core->MaxFramesInFlight;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchainSize;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 1;
	createInfo.pQueueFamilyIndices = &_core->QueueFamilyIndex;
	createInfo.preTransform = _capabilities.currentTransform; // VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR /* VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR */;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = GetPresentMode();
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;
	if (vkCreateSwapchainKHR(_core->Device, &createInfo, nullptr, &Swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("Swapchain creation failed.");
	}
	if (oldSwapchain)
		vkDestroySwapchainKHR(_core->Device, oldSwapchain, nullptr);
	uint32_t count;
	vkGetSwapchainImagesKHR(_core->Device, Swapchain, &count, nullptr);
	Imgs.resize(count);
	vkGetSwapchainImagesKHR(_core->Device, Swapchain, &count, Imgs.data());
	Views.resize(count);
	_framebuffers.resize(Views.size());

	for (uint32_t i = 0; i < count; i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = Imgs[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = surfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		VkImageView view;
		vkCreateImageView(_core->Device, &createInfo, nullptr, &view);
		Views[i] = view;
	}
	for (int i = 0; i < Views.size(); i++)
	{
		VkFramebufferCreateInfo framebufferCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = RenderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &Views[i];
		framebufferCreateInfo.width = _width;
		framebufferCreateInfo.height = _height;
		framebufferCreateInfo.layers = 1;
		vkCreateFramebuffer(_core->Device, &framebufferCreateInfo, nullptr, &_framebuffers[i]);
	}
}

void egx::VulkanSwapchain::CreateRenderPass()
{
	VkAttachmentReference attachmentReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;

	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = GetSurfaceFormat().format;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = _clearswapchain ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Extracted from ImGui vulkan backend
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo passCreateInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	passCreateInfo.attachmentCount = 1;
	passCreateInfo.pAttachments = &attachmentDescription;
	passCreateInfo.subpassCount = 1;
	passCreateInfo.pSubpasses = &subpass;
	passCreateInfo.dependencyCount = 1;
	passCreateInfo.pDependencies = &dependency;
	vkCreateRenderPass(_core->Device, &passCreateInfo, nullptr, &RenderPass);
}

void VulkanSwapchain::CreatePipelineObjects()
{
	_image_sampler = Sampler::FactoryCreate(_core);
	{
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		VkDescriptorSetLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		createInfo.bindingCount = 1;
		createInfo.pBindings = &binding;
		vkCreateDescriptorSetLayout(_core->Device, &createInfo, nullptr, &_descriptor_layout);
	}
	{
		VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		allocateInfo.descriptorPool = _descriptor_set_pool->GetSetPool();
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &_descriptor_layout;
		_descriptor_set.resize(_framebuffers.size());
		for (size_t i = 0; i < _descriptor_set.size(); i++)
			vkAllocateDescriptorSets(_core->Device, &allocateInfo, &_descriptor_set[i]);
		_last_updated_image.resize(_framebuffers.size(), nullptr);
	}
	{
		VkPipelineLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		createInfo.setLayoutCount = 1;
		createInfo.pSetLayouts = &_descriptor_layout;
		vkCreatePipelineLayout(_core->Device, &createInfo, nullptr, &_blit_layout);
	}
	_builtin_vertex = Shader::CreateFromSource(_core, builtin_vertex_shader, shaderc_vertex_shader);
	_builtin_fragment = Shader::CreateFromSource(_core, builtin_fragment_shader, shaderc_fragment_shader);
}

void VulkanSwapchain::CreatePipeline()
{
	if (_blit_gfx)
		vkDestroyPipeline(_core->Device, _blit_gfx, nullptr);
	VkGraphicsPipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	createInfo.stageCount = 2;
	VkPipelineShaderStageCreateInfo stages[2]{};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = _builtin_vertex.GetShader();
	stages[0].pName = "main";

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = _builtin_fragment.GetShader();
	stages[1].pName = "main";
	createInfo.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	createInfo.pVertexInputState = &vertexInputState;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.pInputAssemblyState = &inputAssembly;

	VkPipelineTessellationStateCreateInfo tessellation{VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
	createInfo.pTessellationState = &tessellation;

	VkViewport viewport{};
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.width = float(_width);
	viewport.height = float(_height);
	VkRect2D scissor{};
	scissor.extent = {uint32_t(_width), uint32_t(_height)};
	VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	createInfo.pViewportState = &viewportState;

	VkPipelineRasterizationStateCreateInfo rasterizationState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationState.lineWidth = 1.0f;
	createInfo.pRasterizationState = &rasterizationState;

	VkPipelineMultisampleStateCreateInfo multisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.pMultisampleState = &multisampleState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	createInfo.pDepthStencilState = &depthStencilState;

	VkPipelineColorBlendAttachmentState attachment{};
	attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
								VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo colorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &attachment;
	createInfo.pColorBlendState = &colorBlendState;

	createInfo.pDynamicState = nullptr;
	createInfo.layout = _blit_layout;
	createInfo.renderPass = RenderPass;
	VkResult result = vkCreateGraphicsPipelines(_core->Device, nullptr, 1, &createInfo, nullptr, &_blit_gfx);
	if(result != VK_SUCCESS) {
		LOG(ERR, "Fatal! Could not create swapchain pipeline!");
		ut::DebugBreak();
	}
}
