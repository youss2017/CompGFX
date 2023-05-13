#include <Utility/CppUtility.hpp>
#include <core/egx.hpp>
#include <pipeline/shaders/shader.hpp>
#include <pipeline/pipeline.hpp>
#include <pipeline/RenderGraph.hpp>
#include <pipeline/RenderTarget.hpp>
#include <memory/egxbuffer.hpp>
#include <stb/stb_image_write.h>
#include <window/swapchain.hpp>
#include <pipeline/Sampler.hpp>

#pragma comment(lib, "CompGFX.lib")
using namespace std;
using namespace egx;

int main()
{
	cpp::Logger::GetGlobalLogger().Options.ShowMessageBoxOnError = true;

	auto state = VulkanICDState::Create("Sandbox", true, true, VK_API_VERSION_1_3, &cpp::Logger::GetGlobalLogger(), nullptr);
	auto gpus = state->QueryGPGPUDevices();
	vk::PhysicalDeviceSynchronization2Features synchornizationFeature(true);
	gpus[0].EnabledFeatures.pNext = &synchornizationFeature;
	auto ctx = state->CreateDevice(gpus[0]);
	ctx->FramesInFlight = 3;

	for (auto& gpu : gpus) {
		LOG(INFO, "{} [{}]", gpu.Name, vk::to_string(gpu.Type));
	}

	PlatformWindow window("Hello", 1000, 1000);
	SwapchainController swapchain(ctx, &window);
	swapchain.Invalidate();

	RenderGraph swapGraph(ctx);
	vk::ClearColorValue clearValue;
	clearValue.setFloat32({ 1.0, 0.0, 0.0, 1.0 });
	RenderTarget swapRT(ctx, swapchain, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore, clearValue);
	swapRT.Invalidate();

	SamplerBuilder sampler(ctx);
	sampler.Invalidate();
	egx::GraphicsPipeline gtest(ctx, Shader(ctx, "gtest1.vert"), Shader(ctx, "gtest1.frag"), swapRT);
	gtest.Invalidate();

	Image2D output(ctx, 1024, 1024, vk::Format::eB8G8R8A8Unorm, 1, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal, false);
	output.CreateView(0);
	ComputePipeline raytracing(ctx, Shader(ctx, "raytracing.comp"));
	auto rtd0 = swapGraph.CreateResourceDescriptor(raytracing);
	auto rtd1 = swapGraph.CreateResourceDescriptor(gtest);
	rtd0.SetInput(0, vk::ImageLayout::eGeneral, 0, output);
	rtd1.SetInput(0, vk::ImageLayout::eShaderReadOnlyOptimal, 0, output, sampler.GetSampler());

	swapGraph
		.Add(0, raytracing, [&](vk::CommandBuffer cmd) {
		vk::DependencyInfo info;
		info.setImageMemoryBarriers(vk::ImageMemoryBarrier2()
			.setImage(output.GetHandle())
			.setSrcAccessMask(vk::AccessFlagBits2::eNone)
			.setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
			.setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
			.setOldLayout(output.CurrentLayout)
			.setNewLayout(vk::ImageLayout::eGeneral)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS)));
		cmd.pipelineBarrier2(info);
		output.CurrentLayout = vk::ImageLayout::eGeneral;

		rtd0.Bind(cmd, raytracing.BindPoint(), raytracing.Layout());
		cmd.dispatch(32, 32, 1);

		info = vk::DependencyInfo();
		info.setImageMemoryBarriers(vk::ImageMemoryBarrier2()
			.setImage(output.GetHandle())
			.setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
			.setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
			.setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
			.setOldLayout(output.CurrentLayout)
			.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS)));
		cmd.pipelineBarrier2(info);
		output.CurrentLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			})
		.Add(1, gtest, [&](vk::CommandBuffer cmd) {
				swapRT.Begin(cmd);
				rtd1.Bind(cmd, gtest.BindPoint(), gtest.Layout());
				cmd.draw(6, 1, 0, 0);
				swapRT.End(cmd);
			});


			while (!window.ShouldClose()) {
				PlatformWindow::Poll();
				swapchain.Acquire();
				swapGraph.RunAsync();
				swapchain.Present();
			}

}
