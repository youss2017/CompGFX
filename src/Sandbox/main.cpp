#include <Utility/CppUtility.hpp>
#include <egx/core/egx.hpp>
#include <memory>
#include <random>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include "escapi.h"
#include <glm/mat3x3.hpp>
#include "WorleyNoise.hpp"

using namespace egx;

int main()
{
	LOG(INFO, "SANDBOX");

	std::shared_ptr<EngineCore> engine = std::make_shared<EngineCore>(EngineCoreDebugFeatures::GPUAssisted, false);
	//engine->GetEngineLogger()->Options.ShowMessageBoxOnError = true;
	engine->EstablishDevice(engine->EnumerateDevices()[0]);
	auto& interface = engine->CoreInterface;

	auto window = std::make_shared<PlatformWindow>("Sandbox", 800, 600);
	engine->AssociateWindow(window.get(), 2, true, true);
	auto& swapchain = engine->Swapchain;

	const int width = 320;
	const int height = 240;

	auto sampler = Sampler::FactoryCreate(interface);
	auto img = Image::FactoryCreate(interface,
		VK_IMAGE_ASPECT_COLOR_BIT, width, height, VK_FORMAT_B8G8R8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, true);
	img->createview(0, { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_ONE });

	auto outputImage = Image::FactoryCreate(interface, VK_IMAGE_ASPECT_COLOR_BIT, width, height, VK_FORMAT_B8G8R8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	outputImage->createview(0);

	auto previousInputImage = outputImage->clone();
	previousInputImage->createview(0);
	previousInputImage->setlayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	const int cameraIndex = 0;
	SimpleCapParams params{};
	params.mWidth = width;
	params.mHeight = height;
	std::vector<int> imageBuffer(params.mWidth * params.mHeight);
	params.mTargetBuf = imageBuffer.data();
	setupESCAPI();
	initCapture(cameraIndex, &params);

	std::string cameraName;
	cameraName.resize(256);
	getCaptureDeviceName(cameraIndex, cameraName.data(), (int)cameraName.size());
	LOG(INFOBOLD, "Selected Camera {0}", cameraName);

	// create compute pipeline
	const uint32_t localSizeX = 32;
	const uint32_t localSizeY = 32;
	auto computeShader = Shader2::FactoryCreate(interface, "sample.comp");
	computeShader->SetSpecializationConstant(0, localSizeX);
	computeShader->SetSpecializationConstant(1, localSizeY);
	auto pipeline = Pipeline::FactoryCreate(interface);
	pipeline->invalidate(computeShader);
	pipeline->Layout->Sets[0]->SetImage(0, { img }, {}, { VK_IMAGE_LAYOUT_GENERAL }, { 0 });
	pipeline->Layout->Sets[0]->SetImage(1, { outputImage }, {}, { VK_IMAGE_LAYOUT_GENERAL }, { 0 });
	pipeline->Layout->Sets[0]->SetImage(2, { previousInputImage }, {}, { VK_IMAGE_LAYOUT_GENERAL }, { 0 });
	CommandBuffer Cmd{ interface };
	auto fence = Fence::FactoryCreate(interface, false);

	auto noise = WorleyNoise(interface, 320, 250);
	noise.Generate(10);

	double deltaTime = 0.0;
	struct {
		int algo = 0;
		float threshold = 0.0;
		float rc = 0.0;
		glm::mat3 kernal{1.0};
	} pushblock;
	bool reload = false;
	while (!window->ShouldClose()) {
		auto start = std::chrono::high_resolution_clock::now();
		PlatformWindow::Poll();
		swapchain->Acquire();

		doCapture(cameraIndex);
		//while (!isCaptureDone(cameraIndex)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
		img->write((uint8_t*)imageBuffer.data(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, width, height, 0, width * 4);

		if (reload) {
			LOG(WARNING, "Reloading Compute Pipeline");
			reload = false;
			engine->WaitIdle();
			auto computeShader = Shader2::FactoryCreate(interface, "sample.comp");
			computeShader->SetSpecializationConstant(0, localSizeX);
			computeShader->SetSpecializationConstant(1, localSizeY);
			pipeline->invalidate(computeShader);
			pipeline->Layout->Sets[0]->SetImage(0, { img }, {}, { VK_IMAGE_LAYOUT_GENERAL }, { 0 });
			pipeline->Layout->Sets[0]->SetImage(1, { outputImage }, {}, { VK_IMAGE_LAYOUT_GENERAL }, { 0 });
			pipeline->Layout->Sets[0]->SetImage(2, { previousInputImage }, {}, { VK_IMAGE_LAYOUT_GENERAL }, { 0 });
		}

		auto cmd = Cmd.GetBuffer();

		pipeline->Bind(cmd);
		img->barrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_NONE,
			VK_ACCESS_MEMORY_WRITE_BIT);

		outputImage->barrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_NONE,
			VK_ACCESS_MEMORY_READ_BIT);

		vkCmdPushConstants(cmd, pipeline->Layout->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushblock), &pushblock);
		vkCmdDispatch(cmd, (width + localSizeX - 1) / localSizeX, (height + localSizeY - 1) / localSizeY, 1);

		img->barrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT,
			VK_ACCESS_NONE);

		outputImage->barrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT,
			VK_ACCESS_NONE);

		Cmd.Finalize();
		Cmd.Submit(fence);
		fence->Wait(UINT64_MAX);
		fence->Reset();

		ImGui::Begin("Stats");
		ImGui::Text("Time %.3lf / Fps %.0lf", deltaTime, 1000.0 / deltaTime);
		ImGui::SameLine();
		if (ImGui::Button("Reload")) {
			reload = true;
		}
		static int pointCount = 10;
		if (ImGui::Button("More Noise")) {
			noise.Generate(pointCount);
		}
		ImGui::SameLine();
		ImGui::SliderInt("Point Count", &pointCount, 0, 100);

		ImGui::Image(noise.NoiseImage->GetImGuiTextureID(sampler->GetSampler()), { (float)noise.Width, (float)noise.Height });
		ImGui::End();

		ImGui::Begin("Input Image");
		const char* items[] = { "Pass-Through", "Threshold", "Motion", "Low-Pass Temporal Filtering", "Convolution" };
		ImGui::Combo("Algorithim", &pushblock.algo, items, IM_ARRAYSIZE(items));

		if (pushblock.algo == 1) {
			ImGui::SliderFloat("Threshold", &pushblock.threshold, 0, 2.0);
		}
		if (pushblock.algo == 3) {
			ImGui::SliderFloat("Low-Pass RC Value", &pushblock.rc, 0.0, 1.0);
		}
		if (pushblock.algo == 4) {
			static int kernalId = 0;
			const char* kernalItems[] = { "Blur", "Sharpen", "Horizontal Edge-Detection", "Vertical Edge-Detection" };
			ImGui::Combo("Kernal", &kernalId, kernalItems, IM_ARRAYSIZE(kernalItems));

			if (kernalId == 0) {
				pushblock.kernal = {
					{0.0625, 0.125, 0.0625},
					{0.125, 0.25, 0.125},
					{0.0625, 0.125, 0.0625}
				};
			}
			else if (kernalId == 1) {
				pushblock.kernal = {
					{0.0, -1.0, 0.0},
					{-1.0, 5.0, -1.0},
					{0.0, -1.0, 0.0}
				};
			}
			else if (kernalId == 2) {
				pushblock.kernal = {
					{2.0, 0.0, -2.0},
					{2.0, 0.0, -2.0},
					{2.0, 0.0, -2.0}
				};
			}
			else if (kernalId == 3) {
				pushblock.kernal = {
					{+1.0, +2.0, +1.0},
					{0.0, 0.0, 0.0},
					{-1.0, -2.0, -1.0}
				};
			}
		}

		ImGui::Image(img->GetImGuiTextureID(sampler->GetSampler()), { (float)width, (float)height });
		ImGui::SameLine();
		ImGui::Image(outputImage->GetImGuiTextureID(sampler->GetSampler()), { (float)width, (float)height });
		ImGui::End();

		swapchain->Present();
		auto end = std::chrono::high_resolution_clock::now();
		deltaTime = (end - start).count() * 1e-6;
	}

	deinitCapture(cameraIndex);
	engine->WaitIdle();

}
