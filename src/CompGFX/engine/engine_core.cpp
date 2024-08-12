#include "engine_core.hpp"
#include <thread>
using namespace std;

bool egx::CoreEngine::Startup(int backBufferCount)
{
	_backBufferCount = backBufferCount;
	BackBufferCount = backBufferCount;

	if (backBufferCount < 1) {
		throw std::runtime_error("Back buffer count must be at least 1.");
	}

	ICD = VulkanICDState::Create("EGX Engine", true, false, VK_API_VERSION_1_3, nullptr, nullptr);
	auto devices = ICD->QueryGPGPUDevices();
	if (devices.empty()) {
		throw std::runtime_error("No devices found that support Vulkan.");
	}
	PhysicalDevice = devices[0];
	VkPhysicalDeviceSynchronization2Features features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
	features.synchronization2 = true;
	PhysicalDevice.EnabledFeatures.pNext = &features;
	Device = ICD->CreateDevice(PhysicalDevice, backBufferCount);
	return true;
}

void egx::CoreEngine::LaunchWindow(const std::string& title, int width, int height)
{
	Window = std::make_shared<PlatformWindow>(title, width, height);
	SwapChain = ISwapchainController(Device, Window.get());
	SwapChain.SetupImGui()
		.SetResizeOnWindowResize(true)
		.SetVSync(true)
		.Invalidate();
}

egx::IRenderTarget& egx::CoreEngine::GetSwapChainRenderTarget(bool invalidate)
{
	if (_swapChainRT)
		return _swapChainRT.value();
	if (!Window) {
		throw std::runtime_error("Cannot create render target without window.");
	}
	_swapChainRT = IRenderTarget(Device, 
								SwapChain, 
								vk::ImageLayout::eUndefined, 
								vk::ImageLayout::eColorAttachmentOptimal,
								vk::ImageLayout::ePresentSrcKHR,
								vk::AttachmentLoadOp::eClear,
								vk::AttachmentStoreOp::eStore,
								{});
	_swapChainRT->EnableDearImGui(*Window);
	if (invalidate)
		_swapChainRT->Invalidate();
	return _swapChainRT.value();
}

egx::GraphicsPipelineBuilder& egx::GraphicsPipelineBuilder::SetVertexShader(const std::string& path) {
	auto code = cpp::ReadAllText(path);
	if (!code) {
		throw std::runtime_error(cpp::Format("Could not load shader from disk with path = {}", path));
	}
	return SetVertexShaderCode(*code);
}

egx::GraphicsPipelineBuilder& egx::GraphicsPipelineBuilder::SetVertexShaderCode(const std::string& code) {
	_vertexShaderCode = code;
	return *this;
}

egx::GraphicsPipelineBuilder& egx::GraphicsPipelineBuilder::SetFragmentShader(const std::string& path) {
	auto code = cpp::ReadAllText(path);
	if (!code) {
		throw std::runtime_error(cpp::Format("Could not load shader from disk with path = {}", path));
	}
	return SetFragmentShaderCode(*code);
}

egx::GraphicsPipelineBuilder& egx::GraphicsPipelineBuilder::SetFragmentShaderCode(const std::string& code) {
	_fragmentShaderCode = code;
	return *this;
}

void egx::GraphicsPipelineBuilder::Invalidate(const CoreEngine& engine, const IRenderTarget& renderTarget, bool compileInDebugMode, PipelineSpecification pipelineSpec)
{
	thread vsCompile = thread{ [&]() {
			VertexShader = Shader(engine.Device, _vertexShaderCode, Shader::Type::Vertex, {}, {}, compileInDebugMode);
		}
	};
	thread fsCompile = thread{ [&]() {
			FragmentShader = Shader(engine.Device, _fragmentShaderCode, Shader::Type::Fragment, {}, {}, compileInDebugMode);
		}
	};
	vsCompile.join();
	fsCompile.join();
	Pipeline = IGraphicsPipeline(engine.Device, VertexShader, FragmentShader, renderTarget, pipelineSpec);
	Pipeline.Invalidate();
}

