#include <iostream>
#include <engine/engine_core.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

using namespace std;
using namespace egx;

int main(int argc, char** argv) {

	LOG(INFO, "Hello Engine-Tester.");

	Shader::SetGlobalCacheDirectory("./shaders/spir-v/");

	CoreEngine engine;
	engine.Startup(3);
	engine.LaunchWindow("Engine-Tester", 800, 600);

	auto RT = engine.GetSwapChainRenderTarget(false);
	RT.CreateSingleDepthAttachment(vk::Format::eD32Sfloat, vk::ImageLayout::eUndefined,
									vk::ImageLayout::eDepthStencilAttachmentOptimal,
									vk::ImageLayout::eDepthStencilAttachmentOptimal,
									vk::AttachmentLoadOp::eClear,
									vk::AttachmentStoreOp::eDontCare,
									{});
	RT.Invalidate();

	GraphicsPipelineBuilder pipeline;
	PipelineSpecification spec;
	spec.DepthEnabled = true;
	spec.DepthCompare = vk::CompareOp::eGreater;
	pipeline.SetVertexShader("./shaders/vs.glsl")
		.SetFragmentShader("./shaders/fs.glsl")
		.Invalidate(engine, RT, true, spec);

	ICommandBuffer cmd = engine.CreateCmdBuffer();

	auto semaphore = engine.CreateSemaphore();
	auto fence = engine.CreateFence();

	BufferedMeshContainer teapot(engine.Device);
	teapot.Load("./mesh/teapot.obj", IndicesType::UInt16, { VertexDataOrder::Position, VertexDataOrder::Normal });

	ImGui::SetCurrentContext(RT.GetImGuiContext());

	glm::vec3 offset{};
	float scale = 1.0;
	float freq = 0.0;
	float freq2 = 0.0;
	float freq3 = 0.0;

	while (!engine.Window->ShouldClose()) {
		PlatformWindow::Poll();

		fence.Wait("cmdDone");
		fence.Reset("cmdDone");

		auto acquireImageLock = engine.SwapChain.Acquire();

		cmd.Reset();
		auto c0 = cmd.GetCmd();

		c0.begin(vk::CommandBufferBeginInfo());

		RT.Begin(c0);
		RT.BeginDearImGuiFrame();

		ImGui::SliderFloat3("offset", &offset[0], -5.0, 5.0);
		ImGui::SliderFloat("scale", &scale, 0, 5.0);
		ImGui::InputFloat("Hz", &freq, 0.1, 0.5);
		ImGui::InputFloat("Hz2", &freq2, 0.1, 0.5);
		ImGui::InputFloat("Hz3", &freq3, 0.1, 0.5);
		glm::mat4 transform = glm::translate<float>(glm::mat4(1), offset) 
			* glm::rotate<float>(glm::mat4(1.0), glfwGetTime() * 2.0 * 3.14 * freq, glm::vec3(0.0, 0.0, 1.0))
			* glm::rotate<float>(glm::mat4(1.0), glfwGetTime() * 2.0 * 3.14 * freq2, glm::vec3(0.0, 1.0, 0.0))
			* glm::rotate<float>(glm::mat4(1.0), glfwGetTime() * 2.0 * 3.14 * freq3, glm::vec3(1.0, 0.0, 0.0))
			* glm::scale(glm::mat4(1.0), glm::vec3(scale));

		if (ImGui::Button("Toggle Wireframe mode")) {
			engine.WaitIdle();
			spec.FillMode = vk::PolygonMode(!int(spec.FillMode));
			pipeline.SetVertexShader("./shaders/vs.glsl")
				.SetFragmentShader("./shaders/fs.glsl")
				.Invalidate(engine, RT, true, spec);
		}

		glm::mat4 ortho = glm::ortho<float>(-1, 1, -1, 1, -10, -10);

		pipeline.Pipeline.Bind(c0);
		c0.bindVertexBuffers(0, { teapot.GetVertexBuffer().GetHandle() }, { 0 });
		c0.bindIndexBuffer(teapot.GetIndexBuffer().GetHandle(), 0, vk::IndexType::eUint16);

		c0.pushConstants(pipeline.Pipeline.Layout(), vk::ShaderStageFlagBits::eVertex, sizeof(transform)*0, sizeof(transform), &transform);
		c0.pushConstants(pipeline.Pipeline.Layout(), vk::ShaderStageFlagBits::eVertex, sizeof(transform)*1, sizeof(transform), &ortho);

		c0.drawIndexed(teapot.GetIndicesCount(), 1, 0, 0, 0);

		RT.EndDearImGuiFrame(c0);
		RT.End(c0);

		c0.end();

		auto presentReady = semaphore.GetSemaphore("present-ready");
		auto waitStage = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);

		vk::SubmitInfo submitInfo{};
		submitInfo.setCommandBuffers(c0)
			.setWaitSemaphores(acquireImageLock)
			.setWaitDstStageMask(waitStage)
			.setCommandBuffers(c0)
			.setSignalSemaphores(presentReady);

		engine.Device->Queue.submit(submitInfo, fence.GetFence(true, "cmdDone"));

		engine.SwapChain.Present({ presentReady });
		engine.NextFrame();
	}

	engine.WaitIdle();

	return 0;
}