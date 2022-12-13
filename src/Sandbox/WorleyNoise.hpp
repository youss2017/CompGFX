#pragma once
#include <vector>
#include <egx/core/egx.hpp>

using namespace egx;

struct WorleyNoise {
	int Width{};
	int Height{};
	std::vector<float> Noise;
	ref<Image> NoiseImage;

	ref<VulkanCoreInterface> Interface;
	ref<Pipeline> _WorleyCompute;

	WorleyNoise(const ref<VulkanCoreInterface>& interface, int width, int height);

	void Generate(int pointCount);
};
