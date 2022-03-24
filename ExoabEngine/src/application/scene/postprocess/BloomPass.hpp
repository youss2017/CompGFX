#pragma once
#include "../Scene.hpp"
#include <pipeline/PipelineCS.hpp>
#include <pipeline/Framebuffer.hpp>
#include <shaders/ShaderBinding.hpp>
#include <memory/Texture2.hpp>

namespace Application {

	class BloomPass : public Scene {

	public:
		BloomPass(Framebuffer& fbo, int colorAttachmentIndex, float threshold);
		~BloomPass();

		void ReloadShaders();
		void SetThreshold(float threshold) { mThresholdValue = threshold; }
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);

	private:
		void RecordCommands(uint32_t FrameIndex);

	private:
		float mThresholdValue;
		VkSampler mSampler;
		VkDescriptorPool mPool;
		uint32_t mTextureWidth;
		uint32_t mTextureHeight;
		uint32_t mKernalSizeX;
		uint32_t mKernalSizeY;
	public:
		ITexture2 mDownsampleTexture;
		ITexture2 mUpsampleTexture;
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