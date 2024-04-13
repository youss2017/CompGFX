#include <iostream>
#include <core/egxfoundation.hpp>
#include <scene/font/FontAtlas.hpp>

int main() {

	auto icd = egx::VulkanICDState::Create("Font", true, true, VK_API_VERSION_1_3, nullptr, nullptr);
	auto device = icd->CreateDevice(icd->QueryGPGPUDevices()[0]);
	auto window = egx::PlatformWindow("Font Rendering", 480, 240);
	auto swapchain = egx::ISwapchainController(device, &window);
	
	swapchain.SetupImGui().Invalidate();

	vk::ClearColorValue cc;
	cc.setFloat32({ 0.2f, 0.4f, 0.2f, 0.0f });

	auto renderTarget = egx::IRenderTarget(device, swapchain, vk::ImageLayout::eUndefined,
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, cc);

	renderTarget.EnableDearImGui(window).Invalidate();
	ImGui::SetCurrentContext(renderTarget.GetImGuiContext());

	egx::ScopedCommandBuffer cmd(device);

	auto vert = egx::Shader(device, "font.vert");
	auto frag = egx::Shader(device, "font.frag");
	auto fontStack = egx::IGraphicsPipeline(device, vert, frag, renderTarget);
	fontStack.Invalidate();

	egx::FontAtlas atlasGen;
	atlasGen.LoadTTFont("C:\\Windows\\Fonts\\arial.ttf")
		.SetCharacterSet(L"ABCDEFGHabcdefgh")
		.SetAtlasForMinimalMemoryUsage().SetOptimalSpeed().BuildAtlas(64.0f, false, false);

	atlasGen.SavePng("debug.png");

	auto atlas = egx::Image2D(device, atlasGen.AtlasWidth, atlasGen.AtlasHeight, vk::Format::eR8Unorm, 1, vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal,
		true);
	atlas.SetImageData(0, atlasGen.AtlasBmp.data());

	//auto atlas = egx::Image2D::CreateFromFile(device, "arial_sdf_speed.png", vk::Format::eR8G8B8A8Unorm, 1, 
						//vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal, true);
	atlas.CreateView(0);

	auto sampler = egx::ISamplerBuilder(device);
	sampler.Invalidate();
	auto resourcePool = egx::ResourceDescriptorPool(device);
	auto binding0 = egx::ResourceDescriptor(device, resourcePool, fontStack);
	binding0.SetInput(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, 0, atlas, sampler.GetSampler());

	struct quad {
		glm::vec2 pos;
		glm::vec2 uv;
	};

	const auto& A = atlasGen.CharMap[3];
	float sx = A.start_x / (float)atlasGen.AtlasWidth;
	float sy = A.start_y / (float)atlasGen.AtlasHeight;
	float ex = (A.width / (float)atlasGen.AtlasWidth) + sx;
	float ey = (A.height / (float)atlasGen.AtlasHeight) + sy;
	std::vector<quad> cpuQuadBuffer = {
		{{-0.5f, -0.5f}, {sx, sy}},
		{{0.5f, -0.5f}, {ex, sy}},
		{{-0.5f, 0.5f}, {sx, ey}},
		{{0.5f, 0.5f}, {ex, ey}}
	};

	std::vector<uint16_t> cpuIndexBuffer = {
		0, 1, 2, 2, 1, 3
	};

	auto quadBuffer = egx::Buffer(device, sizeof(quad) * cpuQuadBuffer.size(), egx::MemoryPreset::DeviceAndHost, egx::HostMemoryAccess::Sequential, vk::BufferUsageFlagBits::eVertexBuffer, true);
	auto indexBuffer = egx::Buffer(device, sizeof(uint16_t) * cpuIndexBuffer.size(), egx::MemoryPreset::DeviceAndHost, egx::HostMemoryAccess::Sequential, vk::BufferUsageFlagBits::eIndexBuffer, false);

	indexBuffer.Write(cpuIndexBuffer.data());

	while (!window.ShouldClose()) {
		egx::PlatformWindow::Poll();
		cmd.Reset();
		swapchain.AcquireFullLock();

		quadBuffer.Write(cpuQuadBuffer.data());

		renderTarget.Begin(cmd.Get());
		renderTarget.BeginDearImguiFrame();

		ImGui::Begin("Debuging");
		ImGui::Text("loading...");
		ImGui::End();

		cmd->bindPipeline(fontStack.BindPoint(), fontStack.Pipeline());
		cmd->bindVertexBuffers(0, quadBuffer.GetHandle(), { 0 });
		cmd->bindIndexBuffer(indexBuffer.GetHandle(), 0, vk::IndexType::eUint16);
		binding0.Bind(cmd.Get());

		cmd->drawIndexed(6, 1, 0, 0, 0);

		renderTarget.EndDearImguiFrame(cmd.Get());
		renderTarget.End(cmd.Get());

		cmd.RunNow();
		swapchain.Present();
	}

}