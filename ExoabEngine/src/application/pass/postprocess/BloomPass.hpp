#pragma once
#include "../Pass.hpp"
#include <pipeline/PipelineCS.hpp>
#include <pipeline/Framebuffer.hpp>
#include <shaders/ShaderConnector.hpp>
#include <memory/Texture2.hpp>

namespace Application {

	class BloomPass : public Pass {

	public:
		BloomPass(Framebuffer& fbo, int colorAttachmentIndex, float threshold);
		~BloomPass();

		void ReloadShaders();
		void SetThreshold(float threshold) { mThresholdValue = threshold; }
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);

		void GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime);
		ITexture2 GetDownsampleTexture() { return mBlurTexture; }

	private:
		void RecordCommands(uint32_t FrameIndex);

	private:
		uint32_t mColorAttachmentIndex;
		float mThresholdValue;
		VkQueryPool mQuery;
		VkSampler mSampler;
		VkDescriptorPool mPool;
		DescriptorSet mSet;
		ITexture2 mBlurTexture;

		VkPipelineLayout mLayout;
		VkPipeline mPipeline;

	private:
		Framebuffer mFBO;
	};

}