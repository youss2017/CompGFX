#pragma once
#include "../Scene.hpp"
#include <pipeline/PipelineCS.hpp>
#include <pipeline/Framebuffer.hpp>
#include <shaders/ShaderConnector.hpp>
#include <memory/Texture2.hpp>

namespace Application {

	class BloomPass : public Scene {

	public:
		BloomPass(Framebuffer& fbo, int colorAttachmentIndex, float threshold);
		~BloomPass();

		void ReloadShaders();
		void SetThreshold(float threshold) { mThresholdValue = threshold; }
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);

		void GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime);
		ITexture2 GetDownsampleTexture() { return mDownsampleTexture; }

	private:
		void RecordCommands(uint32_t FrameIndex);

	private:
		float mThresholdValue;
		VkQueryPool mQuery;
		VkSampler mSampler;
		VkDescriptorPool mPool;
		uint32_t mTextureWidth;
		uint32_t mTextureHeight;
		uint32_t mKernalSizeX;
		uint32_t mKernalSizeY;
		ITexture2 mDownsampleTexture;
		ITexture2 mUpsampleTexture;
		VkPipelineLayout mThresholdLayout;
		VkPipeline mThreshold;
		DescriptorSet mThresholdSet;
		VkPipelineLayout mDownsampleLayout;
		VkPipeline mDownsample;
		DescriptorSet mDownsampleSet;
	private:
		Framebuffer mFBO;
	};

}