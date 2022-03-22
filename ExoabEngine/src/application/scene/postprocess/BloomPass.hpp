#pragma once
#include "../Scene.hpp"
#include <pipeline/PipelineCS.hpp>
#include <pipeline/Framebuffer.hpp>
#include <shaders/ShaderBinding.hpp>

namespace Application {

	class BloomPass : public Scene {

	public:
		BloomPass(const Framebuffer& fbo);
		~BloomPass();

		void ReloadShaders();
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);

	private:
		void RecordCommands(uint32_t FrameIndex);

	private:
		VkPipelineLayout mThresholdLayout;
		VkPipeline mThreshold;
		ShaderSet mThresholdSet;
		VkPipelineLayout mDownsampleLayout;
		VkPipeline mDownsample;
		ShaderSet mDownsampleSet;
	private:
		Framebuffer mFBO;
	};

}