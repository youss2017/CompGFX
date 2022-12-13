#include "WorleyNoise.hpp"
#include <random>
#include <chrono>
#include <algorithm>
#include <ranges>
#include <glm/glm.hpp>

WorleyNoise::WorleyNoise(const ref<VulkanCoreInterface>& interface, int width, int height) :
	Interface(interface), Width(width), Height(height) {
	_WorleyCompute = Pipeline::FactoryCreate(interface);
	auto shader = Shader2::FactoryCreate(interface, "worlynoise.comp");
	_WorleyCompute->invalidate(shader);
	NoiseImage = Image::FactoryCreate(interface, VK_IMAGE_ASPECT_COLOR_BIT, width, height, VK_FORMAT_R32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	NoiseImage->createview(0, { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE });
}

void WorleyNoise::Generate(int pointCount)
{
	auto shader = Shader2::FactoryCreate(Interface, "worlynoise.comp");
	_WorleyCompute->invalidate(shader);

	std::vector<glm::vec3> points;
	uint32_t seed = (uint32_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			for (int k = -1; k <= 1; k++) {
				int startX = j * Width;
				int endX = startX + Width;
				int startY = i * Height;
				int endY = startY + Height;
				std::mt19937 randomEngine(seed);
				std::uniform_int_distribution<int> distWidth(startX, endX);
				std::uniform_int_distribution<int> distHeight(startY, endY);
				for (int i = 0; i < pointCount; i++) 
					points.push_back({ (float)distWidth(randomEngine), (float)distHeight(randomEngine), (float)distWidth(randomEngine) });
			}
		}
	}
#if 0
	Noise.resize((size_t)Width * Height, FLT_MAX);

	for (int y = 0; y < Height; y++) {
		for (int x = 0; x < Width; x++) {
			glm::vec3 pixel{ x, y, 0 };
			float val = FLT_MAX;
			for (auto& point : points) {
				val = glm::min(val, glm::distance(pixel, point));
			}
			Noise[y * Width + x] = glm::abs<float>(1.0f - (val / (Width / 2.0)));
		}
	}

	NoiseImage->write((uint8_t*)Noise.data(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Width, Height, 0, Width * sizeof(float));
#else

	struct {
		int32_t g_PointCount{ 0 };
	} pushblock;
	pushblock.g_PointCount = (int32_t)points.size();
	if (points.size() == 0) points.push_back({ 0,0,0 });
	auto pointBuffer = Buffer::FactoryCreate(Interface, sizeof(glm::vec2) * points.size(), egx::memorylayout::stream, BufferType_Storage, false, false);
	pointBuffer->Write(points.data());
	_WorleyCompute->Sets[0]->SetImage(0, { NoiseImage }, {}, { VK_IMAGE_LAYOUT_GENERAL }, { 0 });
	_WorleyCompute->Sets[0]->SetBuffer(1, pointBuffer, 0, 0);

	auto Cmd = CommandBufferSingleUse(Interface);
	auto cmd = Cmd.Cmd;
	
	_WorleyCompute->Bind(cmd);
	NoiseImage->barrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_NONE, VK_ACCESS_MEMORY_WRITE_BIT);
	_WorleyCompute->PushConstants(cmd, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushblock), &pushblock);
	int groupX = (Width + 32 - 1) / 32;
	int groupY = (Height + 32 - 1) / 32;
	vkCmdDispatch(cmd, groupX, groupY, 1);
	NoiseImage->barrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_NONE);
	
	Cmd.Execute();
#endif
}