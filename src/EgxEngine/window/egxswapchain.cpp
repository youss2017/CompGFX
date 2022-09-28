#include "egxswapchain.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <algorithm>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>

egx::VulkanSwapchain::VulkanSwapchain(ref<VulkanCoreInterface>& CoreInterface, void* GlfwWindowPtr, bool VSync, bool SetupImGui, bool ClearSwapchain)
	: _core(CoreInterface), GlfwWindowPtr(GlfwWindowPtr), _vsync(VSync), _imgui(SetupImGui), _clearswapchain(ClearSwapchain)
{
	VkBool32 Supported{};
	glfwCreateWindowSurface(_core->Instance, (GLFWwindow*)GlfwWindowPtr, nullptr, &Surface);
	vkGetPhysicalDeviceSurfaceSupportKHR(CoreInterface->PhysicalDevice.Id, CoreInterface->QueueFamilyIndex, Surface, &Supported);
	if (!Supported) {
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

	glfwGetFramebufferSize((GLFWwindow*)GlfwWindowPtr, &_width, &_height);
	CreateRenderPass();
	CreateSwapchain(_width, _height);
	VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	vkCreateFence(_core->Device, &createInfo, nullptr, &_dummyfence);

	// ImGui_ImplVulkanH_Window window{};
	//ImGui_ImplVulkanH_CreateOrResizeWindow(_core->Instance, _core->PhysicalDevice.Id, _core->Device, &window, _core->QueueFamilyIndex, nullptr, )

	_pool = CreateCommandPool(_core, true, false, false, false);
	_cmd = _pool->AllocateBufferFrameFlightMode(true);
	_presentlock = CreateSemaphore(_core, true);
	_poollock = CreateFence(_core, true, true);

	if (_imgui) {
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
			[](const char* function_name, void* user_data) throw()->PFN_vkVoidFunction
			{
				ImGUI_LoadFunctions_UserData* pUserData = (ImGUI_LoadFunctions_UserData*)user_data;
				PFN_vkVoidFunction proc = (PFN_vkVoidFunction)vkGetDeviceProcAddr(pUserData->device, function_name);
				if (!proc)
					return vkGetInstanceProcAddr(pUserData->instance, function_name);
				else
					return proc;
			},
			&ImGui_Load_UserData);
#endif

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
			{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000} };

		VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		createInfo.maxSets = 1000;
		createInfo.poolSizeCount = (uint32_t)poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();
		vkCreateDescriptorPool(CoreInterface->Device, &createInfo, nullptr, &_imguipool);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
		// io.ConfigDockingWithShift = true;

		VkQueue queue;
		vkGetDeviceQueue(CoreInterface->Device, CoreInterface->QueueFamilyIndex, 0, &queue);
		ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)this->GlfwWindowPtr, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = CoreInterface->Instance;
		init_info.PhysicalDevice = CoreInterface->PhysicalDevice.Id;
		init_info.Device = CoreInterface->Device;
		init_info.QueueFamily = CoreInterface->QueueFamilyIndex;
		init_info.Queue = queue;
		init_info.PipelineCache = NULL;
		init_info.DescriptorPool = _imguipool;
		init_info.Allocator = NULL;
		init_info.MinImageCount = this->_core->MaxFramesInFlight;
		init_info.ImageCount = this->_core->MaxFramesInFlight;
		init_info.CheckVkResultFn = NULL;
		ImGui_ImplVulkan_Init(&init_info, RenderPass);
		// ImFont* fftt = io.Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCodePL-SemiBold.ttf", 14.5f);

		// Upload Fonts
		{
			// Use any command queue
			VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			VkCommandPool command_pool;
			vkCreateCommandPool(CoreInterface->Device, &createInfo, nullptr, &command_pool);
			VkCommandBuffer command_buffer;
			VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
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

egx::VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& move) noexcept
{
	this->~VulkanSwapchain();
	memcpy(this, &move, sizeof(VulkanSwapchain));
	memset(&move, 0, sizeof(VulkanCoreInterface));
}

egx::VulkanSwapchain& egx::VulkanSwapchain::operator=(VulkanSwapchain& move) noexcept
{
	this->~VulkanSwapchain();
	memcpy(this, &move, sizeof(VulkanSwapchain));
	memset(&move, 0, sizeof(VulkanCoreInterface));
	return *this;
}

egx::VulkanSwapchain::~VulkanSwapchain()
{
	if (_imgui) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		vkDestroyDescriptorPool(_core->Device, _imguipool, nullptr);
	}
	if (RenderPass) vkDestroyRenderPass(_core->Device, RenderPass, nullptr);
	if (Swapchain) vkDestroySwapchainKHR(_core->Device, Swapchain, nullptr);
	if (Surface) vkDestroySurfaceKHR(_core->Instance, Surface, nullptr);
	if (_dummyfence) vkDestroyFence(_core->Device, _dummyfence, nullptr);
	for (auto view : Views) vkDestroyImageView(_core->Device, view, nullptr);
	for (auto fbo : _framebuffers) vkDestroyFramebuffer(_core->Device, fbo, nullptr);
}

void egx::VulkanSwapchain::Acquire(bool block, VkSemaphore ReadySemaphore, VkFence ReadyFence)
{
	assert(block || ReadySemaphore || ReadyFence);
	VkResult result;
	if (block) {
		result = vkAcquireNextImageKHR(_core->Device, Swapchain, UINT64_MAX, nullptr, _dummyfence, &_core->CurrentFrame);
		if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
			vkWaitForFences(_core->Device, 1, &_dummyfence, VK_TRUE, UINT64_MAX);
		vkResetFences(_core->Device, 1, &_dummyfence);
	}
	else {
		assert(ReadySemaphore || ReadyFence);
		result = vkAcquireNextImageKHR(_core->Device, Swapchain, UINT64_MAX, ReadySemaphore, ReadyFence, &_core->CurrentFrame);
	}
	if (result == VK_ERROR_OUT_OF_DATE_KHR) Resize();
	if (_imgui) {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
}

void egx::VulkanSwapchain::Present(ref<Image>& image, uint32_t viewIndex, const std::vector<VkSemaphore>& WaitSemaphores)
{
	assert(!_clearswapchain && "Cannot clear swapchain when presentating an image because the copied image will be cleared.");
	_poollock->Wait(UINT64_MAX);
	_poollock->Reset();
	_pool->Reset(0);
	uint32_t frame = _core->CurrentFrame;

	StartCommandBuffer(_cmd[frame], 0);
	DInsertDebugLabel(_core, _cmd[frame], frame, "Swapchain Present", 0.5f, 0.4f, 0.65f);

	VkImageMemoryBarrier barriers[2]{};
	barriers[0] = Image::Barrier(Imgs[frame], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT);

	if (_imgui) {
		vkCmdPipelineBarrier(_cmd[frame], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, barriers);
	}
	else {
		barriers[1] = Image::Barrier(Imgs[frame], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE);
		vkCmdPipelineBarrier(_cmd[frame], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 2, barriers);
	}

	VkImageBlit region{};
	region.srcOffsets[0] = region.dstOffsets[0] = { 0, 0, 0 };
	region.srcOffsets[1] = { (int32_t)image->Width, (int32_t)image->Height, 1 };
	region.dstOffsets[1] = { _width, _height, 1 };
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.srcSubresource.aspectMask = image->ImageAspect;
	region.srcSubresource.layerCount = 1;
	vkCmdBlitImage(_cmd[frame], image->Img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Imgs[frame], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);
	if (_imgui) {
		VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = RenderPass;
		beginInfo.framebuffer = _framebuffers[frame];
		beginInfo.renderArea = { {}, {(uint32_t)_width, (uint32_t)_height} };
		vkCmdBeginRenderPass(_cmd[frame], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		ImGui::Render();
		auto draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, _cmd[frame]);
		vkCmdEndRenderPass(_cmd[frame]);
	}

	if (_imgui) {
		auto& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			auto backup_context = ImGui::GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			ImGui::SetCurrentContext(backup_context);
		}
	}

	vkEndCommandBuffer(_cmd[frame]);

	std::vector<VkPipelineStageFlags> waitStages(WaitSemaphores.size());
	std::fill(waitStages.begin(), waitStages.end(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	DEndDebugLabel(_core, _cmd[frame]);
	SubmitCmdBuffers(_core, { _cmd[frame] }, WaitSemaphores, waitStages, { _presentlock->GetSemaphore() }, { _poollock->GetFence() });

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_presentlock->GetSemaphore();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &Swapchain;
	presentInfo.pImageIndices = &_core->CurrentFrame;
	vkQueuePresentKHR(_core->Queue, &presentInfo);
}

void egx::VulkanSwapchain::Present(const std::vector<VkSemaphore>& WaitSemaphores)
{
	assert(_imgui && "To use default Present() you must have enabled imgui otherwise you are presenting nothing.");
	_poollock->Wait(UINT64_MAX);
	_poollock->Reset();
	_pool->Reset(0);
	uint32_t frame = _core->CurrentFrame;

	StartCommandBuffer(_cmd[frame], 0);
	VkClearValue clear{};
	VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	beginInfo.renderPass = RenderPass;
	beginInfo.framebuffer = _framebuffers[frame];
	beginInfo.renderArea = { {}, {(uint32_t)_width, (uint32_t)_height} };
	beginInfo.clearValueCount = _clearswapchain ? 1 : 0;
	beginInfo.pClearValues = &clear;
	vkCmdBeginRenderPass(_cmd[frame], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	if (_imgui) {
		ImGui::Render();
		auto draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, _cmd[frame]);
	}
	vkCmdEndRenderPass(_cmd[frame]);
	vkEndCommandBuffer(_cmd[frame]);

	if (_imgui) {
		auto& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			auto backup_context = ImGui::GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			ImGui::SetCurrentContext(backup_context);
		}
	}

	std::vector<VkPipelineStageFlags> waitStages(WaitSemaphores.size());
	std::fill(waitStages.begin(), waitStages.end(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	SubmitCmdBuffers(_core, { _cmd[frame] }, WaitSemaphores, waitStages, { _presentlock->GetSemaphore() }, { _poollock->GetFence() });

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_presentlock->GetSemaphore();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &Swapchain;
	presentInfo.pImageIndices = &_core->CurrentFrame;
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
}

void egx::VulkanSwapchain::Resize()
{
	int w, h;
	glfwGetFramebufferSize((GLFWwindow*)GlfwWindowPtr, &w, &h);
	if (w == 0 || h == 0) return;
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
		{ VK_FORMAT_B8G8R8_UINT},
		{ VK_FORMAT_R8G8B8_UINT},
		{ VK_FORMAT_B8G8R8_SINT},
		{ VK_FORMAT_R8G8B8_SINT},
		{ VK_FORMAT_R8G8B8_UNORM}
	};
	std::vector<VkFormat> supportedList;
	for (auto& surfaceFormat : _surfaceFormats) {
		supportedList.push_back(surfaceFormat.format);
	}
	for (int i = 0; i < desiredList.size(); i++) {
		auto temp = std::find(supportedList.begin(), supportedList.end(), desiredList[i]);
		if (temp != supportedList.end()) return _surfaceFormats[temp - supportedList.begin()];
	}
	return _surfaceFormats[0];
}

VkPresentModeKHR egx::VulkanSwapchain::GetPresentMode()
{
	if (!_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_MAILBOX_KHR)) return VK_PRESENT_MODE_MAILBOX_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR)) return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	if (std::count(_presentationModes.begin(), _presentationModes.end(), VK_PRESENT_MODE_FIFO_KHR)) return VK_PRESENT_MODE_FIFO_KHR;
	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

void egx::VulkanSwapchain::CreateSwapchain(int width, int height)
{
	vkQueueWaitIdle(_core->Queue);
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_core->PhysicalDevice.Id, Surface, &_cabilities);
	VkSurfaceFormatKHR surfaceFormat = GetSurfaceFormat();
	_width = width;
	_height = height;

	for (auto view : Views) vkDestroyImageView(_core->Device, view, nullptr);
	for (auto fbo : _framebuffers) vkDestroyFramebuffer(_core->Device, fbo, nullptr);

	VkExtent2D swapchainSize =
	{
		std::clamp<uint32_t>(width, _cabilities.minImageExtent.width, _cabilities.maxImageExtent.width),
		std::clamp<uint32_t>(height, _cabilities.minImageExtent.height, _cabilities.maxImageExtent.height)
	};
	_width = swapchainSize.width;
	_height = swapchainSize.height;

	_core->MaxFramesInFlight = std::max<uint32_t>(_core->MaxFramesInFlight, _cabilities.minImageCount);
	assert(_core->MaxFramesInFlight <= 5);

	VkSwapchainKHR oldSwapchain = Swapchain;
	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
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
	createInfo.preTransform = _cabilities.currentTransform; //VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR /* VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR */;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = GetPresentMode();
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;
	if (vkCreateSwapchainKHR(_core->Device, &createInfo, nullptr, &Swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Swapchain creation failed.");
	}
	if (oldSwapchain) vkDestroySwapchainKHR(_core->Device, oldSwapchain, nullptr);
	uint32_t count;
	vkGetSwapchainImagesKHR(_core->Device, Swapchain, &count, nullptr);
	Imgs.resize(count);
	vkGetSwapchainImagesKHR(_core->Device, Swapchain, &count, Imgs.data());
	Views.resize(count);
	_framebuffers.resize(Views.size());

	for (uint32_t i = 0; i < count; i++) {
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
	for (int i = 0; i < Views.size(); i++) {
		VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
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

void egx::VulkanSwapchain::CreateRenderPass() {
	VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

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

	VkRenderPassCreateInfo passCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	passCreateInfo.attachmentCount = 1;
	passCreateInfo.pAttachments = &attachmentDescription;
	passCreateInfo.subpassCount = 1;
	passCreateInfo.pSubpasses = &subpass;
	passCreateInfo.dependencyCount = 1;
	passCreateInfo.pDependencies = &dependency;
	vkCreateRenderPass(_core->Device, &passCreateInfo, nullptr, &RenderPass);
}
