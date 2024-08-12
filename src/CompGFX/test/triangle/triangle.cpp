#include <core/egx.hpp>
#include <window/PlatformWindow.hpp>
#include <window/swapchain.hpp>
#include <pipeline/pipeline.hpp>
#include <pipeline/RenderTarget.hpp>
#include <glm/glm.hpp>
#include <algorithm>

using namespace egx;
using namespace std;
using namespace glm;

static const char* VertexShader = R"(
#version 450 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_col;
layout (location = 0) out vec3 out_col;
void main() {
out_col = in_col;
gl_Position = vec4(in_pos, 1.0);
}
)";

static const char* FragmentShader = R"(
#version 450 core
layout (location = 0) in vec3 in_col;
layout (location = 0) out vec3 out_frag;
void main() {
out_frag = in_col;
}
)";

void triangle_main() {

	auto icd = VulkanICDState::Create("Triangle Test", true, true, VK_API_VERSION_1_2, nullptr, nullptr);
	auto device = icd->CreateDevice(icd->QueryGPGPUDevices()[0]);
	auto window = PlatformWindow("Triangle Test", 400, 360);
	auto swapchain = ISwapchainController(device, &window);
	swapchain.Invalidate();

	struct vertex {
		vec3 position;
		vec3 color;
	};

	vertex vertices[] = {
		{ { -0.5, -0.5, 0.0 }, { 0.0, 1.0, 1.0 } },
		{ { 0.0, 0.5, 0.0 }, { 1.0, 1.0, 0.0 } },
		{ { 0.5, -0.5, 0.0 }, { 1.0, 0.0, 1.0 } }
	};

	auto vertexBuffer = Buffer(device, sizeof(vertices), MemoryPreset::DeviceOnly, HostMemoryAccess::None, vk::BufferUsageFlagBits::eVertexBuffer, false);
	vertexBuffer.Write(vertices);

	auto renderTarget = IRenderTarget(device, swapchain, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, {});
	renderTarget.Invalidate();
	
	auto pipeline = IGraphicsPipeline(device, 
		{ device, VertexShader, Shader::Type::Vertex }, 
		{ device, FragmentShader, Shader::Type::Fragment }, 
		renderTarget);
	pipeline.Invalidate();

	vector<vk::CommandPool> cmdPools(device->FramesInFlight);
	generate(cmdPools.begin(), cmdPools.end(), [&] { return device->Device.createCommandPool({}); });
	vector<vk::CommandBuffer> cmds;
	for(auto& pool : cmdPools)
		cmds.push_back(device->Device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
		.setCommandPool(pool)
		.setCommandBufferCount(1)
		.setLevel(vk::CommandBufferLevel::ePrimary))[0]);

	vector<vk::Semaphore> renderedSemaphore(device->FramesInFlight);
	generate(renderedSemaphore.begin(), renderedSemaphore.end(), [&] { return device->Device.createSemaphore({}); });
	vector<vk::Fence> fences(device->FramesInFlight);
	generate(fences.begin(), fences.end(), [&] { return device->Device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled)); });

	while (!window.ShouldClose()) {
		PlatformWindow::Poll();
		auto acquireSemaphore = swapchain.Acquire();

		auto frame = device->CurrentFrame;
		device->Device.waitForFences(fences[frame], true, 1e9);
		device->Device.resetFences(fences[frame]);
		device->Device.resetCommandPool(cmdPools[frame]);

		cmds[frame].begin(vk::CommandBufferBeginInfo());
		cmds[frame].bindPipeline(pipeline.BindPoint(), pipeline.Pipeline());
		cmds[frame].bindVertexBuffers(0, vertexBuffer.GetHandle(), { 0 });
		renderTarget.Begin(cmds[frame]);
		cmds[frame].draw(3, 1, 0, 0);
		renderTarget.End(cmds[frame]);
		cmds[frame].end();

		vk::PipelineStageFlags waitDstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		device->Queue.submit(vk::SubmitInfo()
			.setCommandBuffers(cmds[frame])
			.setWaitSemaphores(acquireSemaphore)
			.setWaitDstStageMask(waitDstStage)
			.setSignalSemaphores(renderedSemaphore[frame]));

		swapchain.Present({ renderedSemaphore[frame] });
		this_thread::sleep_for(chrono::milliseconds(1));
	}

}
