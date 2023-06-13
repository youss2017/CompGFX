#include "swapchain.hpp"

using namespace std;
using namespace egx;

ISwapchainController::ISwapchainController(const DeviceCtx &pCtx, PlatformWindow *pTargetWindow, vk::Format format)
{
	m_Data = make_shared<ISwapchainController::DataWrapper>();
	m_Data->m_Ctx = pCtx;
	m_Data->m_Window = pTargetWindow;
	m_Data->m_PresentWait.resize(1);
	auto physicalDevice = m_Data->m_Ctx->PhysicalDeviceQuery.PhysicalDevice;

	VkSurfaceKHR surface;
	auto Result = glfwCreateWindowSurface(pCtx->ICDState->Instance(), m_Data->m_Window->GetWindow(), nullptr, &surface);
	if (Result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create window surface");
	}
	m_Data->m_Surface = surface;

	m_Data->m_FramesInFlight = pCtx->FramesInFlight;
	m_Data->m_SurfaceFormats = physicalDevice.getSurfaceFormatsKHR(m_Data->m_Surface);
	m_Data->m_SurfaceFormat = m_Data->m_SurfaceFormats[0];
	m_Data->m_Width = pTargetWindow->GetWidth();
	m_Data->m_Height = pTargetWindow->GetHeight();
	m_Data->m_Format = m_Data->m_SurfaceFormats[0].format;
	LOG(WARNING, "(TODO) Make swapchain controller format dynamic.");
}

void ISwapchainController::Invalidate(bool blockQueue)
{
	if (blockQueue)
		m_Data->m_Ctx->Queue.waitIdle();

	auto physicalDevice = m_Data->m_Ctx->PhysicalDeviceQuery.PhysicalDevice;
	auto props = physicalDevice.getProperties();

	for (vk::Semaphore imageReady : m_Data->m_ImageReady)
	{
		m_Data->m_Ctx->Device.destroySemaphore(imageReady);
	}
	m_Data->m_ImageReady.clear();

	auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(m_Data->m_Surface);
	uint32_t backBufferCount = std::min<int>(surfaceCapabilities.maxImageCount, std::max<int>(m_Data->m_FramesInFlight, surfaceCapabilities.minImageCount));
	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.surface = m_Data->m_Surface;
	createInfo.minImageCount = backBufferCount;
	createInfo.imageFormat = m_Data->m_SurfaceFormat.format;
	createInfo.imageColorSpace = m_Data->m_SurfaceFormat.colorSpace;
	createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	createInfo.queueFamilyIndexCount = 1;
	createInfo.pQueueFamilyIndices = (uint32_t *)&m_Data->m_Ctx->GraphicsQueueFamilyIndex;
	createInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eInherit;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.imageExtent.width = m_Data->m_Width;
	createInfo.imageExtent.height = m_Data->m_Height;
	createInfo.imageArrayLayers = 1;
	createInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	createInfo.setClipped(false);
	createInfo.setOldSwapchain(m_Data->m_Swapchain);

	createInfo.setPresentMode(vk::PresentModeKHR::eImmediate);

	if (m_Data->m_VSync)
	{
		auto presentMode = physicalDevice.getSurfacePresentModesKHR(m_Data->m_Surface);
		if (std::ranges::any_of(presentMode, [](auto &x)
								{ return x == vk::PresentModeKHR::eFifoRelaxed; }))
		{
			createInfo.setPresentMode(vk::PresentModeKHR::eFifoRelaxed);
		}
		else if (std::ranges::any_of(presentMode, [](auto &x)
									 { return x == vk::PresentModeKHR::eFifo; }))
		{
			createInfo.setPresentMode(vk::PresentModeKHR::eFifo);
		}
		else if (std::ranges::any_of(presentMode, [](auto &x)
									 { return x == vk::PresentModeKHR::eMailbox; }))
		{
			createInfo.setPresentMode(vk::PresentModeKHR::eMailbox);
		}
	}

	auto errorCode = m_Data->m_Ctx->Device.createSwapchainKHR(&createInfo, nullptr, &m_Data->m_Swapchain);
	if (errorCode != vk::Result::eSuccess)
	{
		auto error = cpp::Format("Could not create swapchain, VkResult {}", vk::to_string(errorCode));
		throw std::runtime_error(error.c_str());
	}

	for(uint32_t i = 0; i < m_Data->m_Ctx->FramesInFlight; i++) {
		m_Data->m_ImageReady.push_back(m_Data->m_Ctx->Device.createSemaphore(vk::SemaphoreCreateInfo()));
	}

}

void egx::ISwapchainController::Resize(int width, int height, bool blockQueue)
{
	m_Data->m_Width = width, m_Data->m_Height = height;
	Invalidate(blockQueue);
	for (auto& [callback, pUserData] : m_Data->m_ResizeCallbacks)
		callback->_CallbackProtocol(pUserData);
}

vk::Semaphore ISwapchainController::Acquire()
{
	const vk::Semaphore imageReadySemaphore = m_Data->m_ImageReady[m_Data->m_Ctx->CurrentFrame];
	// We wait until the swapchain can give us the next image
	const auto errorCode = m_Data->m_Ctx->Device.acquireNextImageKHR(m_Data->m_Swapchain, numeric_limits<uint64_t>::max(), imageReadySemaphore, {}, &m_Data->m_CurrentBackBufferIndex);
	m_Data->m_Ctx->CurrentFrame = m_Data->m_Ctx->CurrentFrame;

	if (errorCode == vk::Result::eErrorOutOfDateKHR || errorCode == vk::Result::eSuboptimalKHR)
	{
		if (m_Data->m_ResizeOnWindowResize)
		{
			Resize(m_Data->m_Window->GetWidth(), m_Data->m_Window->GetHeight(), true);
			return Acquire();
		}
	}
	else if (errorCode != vk::Result::eSuccess && errorCode != vk::Result::eErrorOutOfDateKHR && errorCode != vk::Result::eSuboptimalKHR)
	{
		// (TODO) Alert user
	}
	m_Data->m_PresentWait[0] = imageReadySemaphore;
	return imageReadySemaphore;
}

uint32_t ISwapchainController::AcquireFullLock()
{
	if (VkFence(m_Data->m_AcquireFullLock) == nullptr)
	{
		m_Data->m_AcquireFullLock = m_Data->m_Ctx->Device.createFence({});
	}
	const auto imageReadySemaphore = m_Data->m_ImageReady[m_Data->m_Ctx->CurrentFrame];

	const auto errorCode = m_Data->m_Ctx->Device.acquireNextImageKHR(m_Data->m_Swapchain, UINT64_MAX, imageReadySemaphore, m_Data->m_AcquireFullLock, &m_Data->m_CurrentBackBufferIndex);
	m_Data->m_Ctx->Device.waitForFences(m_Data->m_AcquireFullLock, true, UINT64_MAX);
	m_Data->m_Ctx->Device.resetFences(m_Data->m_AcquireFullLock);

	if (errorCode == vk::Result::eErrorOutOfDateKHR || errorCode == vk::Result::eSuboptimalKHR)
	{
		if (m_Data->m_ResizeOnWindowResize)
		{
			Resize(m_Data->m_Window->GetWidth(), m_Data->m_Window->GetHeight(), true);
			return AcquireFullLock();
		}
	}
	else if (errorCode != vk::Result::eSuccess && errorCode != vk::Result::eSuboptimalKHR)
	{
		// (TODO) Alert user
	}

	m_Data->m_PresentWait[0] = imageReadySemaphore;
	return m_Data->m_CurrentBackBufferIndex;
}

void ISwapchainController::Present(const std::vector<vk::Semaphore> &presentReadySemaphore)
{
	vk::PresentInfoKHR presentInfo;
	presentInfo.setWaitSemaphores(presentReadySemaphore)
				.setSwapchains(m_Data->m_Swapchain)
				.setImageIndices(m_Data->m_CurrentBackBufferIndex);
	auto result = m_Data->m_Ctx->Queue.presentKHR(presentInfo);
	if (result != vk::Result::eSuccess) {
		LOG(WARNING, "vkQueuePresentKHR(...) = ", vk::to_string(result));
	}
	m_Data->m_Ctx->NextFrame();
}

ISwapchainController::DataWrapper::~DataWrapper()
{
	if (m_Ctx && m_Swapchain)
	{
		m_Ctx->Queue.waitIdle();
		for(vk::Semaphore imageReady : m_ImageReady)
			m_Ctx->Device.destroySemaphore(imageReady);
		m_Ctx->Device.destroySwapchainKHR(m_Swapchain);
		m_Ctx->ICDState->Instance().destroySurfaceKHR(m_Surface);
		auto currentContext = ImGui::GetCurrentContext();
		LOG(ERR, "(TODO) Implement ImGui CleanUp");
	}
}
