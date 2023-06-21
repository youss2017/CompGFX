#include "BitmapWindow.hpp"
#include <scene/IScene.hpp>
#include <pipeline/pipeline.hpp>
#include <pipeline/RenderGraph.hpp>
#include <pipeline/RenderTarget.hpp>
#include <glm/vec4.hpp>

#include "quad.vert"
#include "quad.frag"

using namespace std;
using namespace egx;
using namespace glm;

constexpr uint32_t BitmapWindow_Address = 0x10FF;

struct Quad {
	vec4 position;
	vec4 color;
};
namespace egx {
	class TriangleStage : public IRenderStage {
	public:
		TriangleStage(const DeviceCtx& ctx, const IDataRegistry& reg) : IRenderStage(ctx, reg), m_Sampler(ctx) {}

		virtual void Initialize(bool first_load) override {
			auto rt = m_Registry.Read<IRenderTarget>(DataRegistryTable::RENDER_TARGET);
			m_Pipeline = IGraphicsPipeline(m_Ctx, { m_Ctx, quad_vertex_shader, Shader::Type::Vertex }, { m_Ctx, quad_fragment_shader, Shader::Type::Fragment }, rt);
			m_Pipeline.Invalidate();

			auto window = (BitmapWindow*)m_Registry.Read<size_t>(BitmapWindow_Address);
			m_Width = window->GetWidth(), m_Height = window->GetHeight();

			m_Image = Image2D(m_Ctx, m_Width, m_Height, vk::Format::eB8G8R8A8Unorm, 1, vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal, true);
			m_Image.CreateView(0);
			m_Sampler.Invalidate();

			m_QuadBuffer = Buffer(m_Ctx, sizeof(Quad), MemoryPreset::DeviceAndHost, HostMemoryAccess::Default, vk::BufferUsageFlagBits::eStorageBuffer, true);
			m_Pool = ResourceDescriptorPool(m_Ctx);
			m_Descriptor = ResourceDescriptor(m_Ctx, m_Pool, m_Pipeline);
			m_Descriptor.SetInput(0, vk::ImageLayout::eShaderReadOnlyOptimal, 0, m_Image, m_Sampler.GetSampler());
			m_Descriptor.SetInput(1, m_QuadBuffer);
		}

		virtual void Process(vk::CommandBuffer cmd) override {
			float resolution[2] = { m_Width, m_Height };

			auto window = (BitmapWindow*)m_Registry.Read<size_t>(BitmapWindow_Address);
			m_Image.SetImageData(0, window->GetBackBuffer().data());

			// sample quad
			Quad quad{};
			quad.position = { 0, 0, m_Width, m_Height };
			m_QuadBuffer.Write(&quad);

			cmd.bindPipeline(m_Pipeline.BindPoint(), m_Pipeline.Pipeline());
			cmd.pushConstants(m_Pipeline.Layout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(resolution), resolution);
			m_Descriptor.Bind(cmd);
			cmd.draw(6, 1, 0, 0);

			//ImGui::SetNextWindowPos({ 0, 0 });
			//ImGui::SetNextWindowSize({ 100, 80 });
		/*	ImGui::Begin("Metrics");
			ImGui::Text("FPS %d", fps);
			ImGui::SliderInt("X: ", &player.position.x, 0, width / block_size);
			ImGui::SliderInt("Y: ", &player.position.y, 0, height / block_size);
			ImGui::End();*/
		}

	private:
		IGraphicsPipeline m_Pipeline;
		Buffer m_QuadBuffer;
		Image2D m_Image;
		ResourceDescriptorPool m_Pool;
		ResourceDescriptor m_Descriptor;
		ISamplerBuilder m_Sampler;
		int m_Width = 0;
		int m_Height = 0;
	};

	class BitmapScene : public IScene {
	public:
		BitmapScene(const DeviceCtx& ctx, const IRenderTarget& rt, BitmapWindow* window) : IScene(ctx) {
			SetRenderTarget(rt);
			m_Registry.Store(BitmapWindow_Address, (size_t)window);
			shared_ptr<TriangleStage> triangle_stage = make_shared<TriangleStage>(ctx, m_Registry);
			AddRenderStage(triangle_stage);
			Initialize(true);
		}
	};
}

egx::BitmapWindow::BitmapWindow(const DeviceCtx& ctx, std::string title, int width, int height)
	: PlatformWindow(title, width, height), m_BackBuffer(width * height)
{
	m_Swapchain = ISwapchainController(ctx, this);
	m_Swapchain.SetupImGui().Invalidate();
	
	vk::ClearValue clear;
	clear.color.setFloat32({ 0., 0., 0., 1.0 });

	IRenderTarget rt(ctx, m_Swapchain, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clear);
	rt.EnableDearImGui(*this).Invalidate();

	m_Scene = new BitmapScene(ctx, rt, this);
}

BitmapWindow::~BitmapWindow() {
	delete m_Scene;
}

void egx::BitmapWindow::SwapBuffers()
{
	m_Swapchain.Acquire();
	m_Scene->Process();
	m_Swapchain.Present();
}
