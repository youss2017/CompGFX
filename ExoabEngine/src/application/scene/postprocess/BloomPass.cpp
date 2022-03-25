#include "BloomPass.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <glm/glm.hpp>

extern vk::VkContext gContext;

struct DownsamplePushblock {
	glm::uint u_SourceMiplevel;
	glm::vec2 u_SourceMipSize;
	glm::vec2 u_SourceTexelSize;
	glm::vec2 u_DestMipSize;
	glm::vec2 u_DestTexelSize;
};

Application::BloomPass::BloomPass(Framebuffer& fbo, int colorAttachmentIndex, float threshold) : Scene(gContext->defaultDevice, true), mFBO(fbo), mThresholdValue(threshold) {
	Texture2DSpecification spec;
	spec.m_Width = fbo.m_width >> 1;
	spec.m_Height = fbo.m_height >> 1;
	spec.mLayers = 1;
	spec.m_TextureUsage = TextureUsage::TEXTURE;
	spec.m_Samples = TextureSamples::MSAA_1;
	spec.m_Format = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	spec.mFlags = 0;
	spec.mUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	spec.mTiling = VK_IMAGE_TILING_OPTIMAL;
	spec.m_GenerateMipMapLevels = true;
	spec.m_CreatePerFrame = true;
	spec.m_LazilyAllocate = false;
	spec.mCreateViewPerMip = true;
	mDownsampleTexture = Texture2_Create(gContext, spec);
	mUpsampleTexture = Texture2_Create(gContext, spec);
	mTextureWidth = spec.m_Width;
	mTextureHeight = spec.m_Height;
	mKernalSizeX = 32;
	mKernalSizeY = 32;
	Shader thresholdShader = Shader(gContext, "assets/shaders/bloom/threshold.comp");
	thresholdShader.SetSpecializationConstant<int>(0, fbo.m_width);
	thresholdShader.SetSpecializationConstant<int>(1, fbo.m_height);
	thresholdShader.SetSpecializationConstant<int>(2, mTextureWidth);
	thresholdShader.SetSpecializationConstant<int>(3, mTextureHeight);
	thresholdShader.SetSpecializationConstant<int>(4, mKernalSizeX);
	thresholdShader.SetSpecializationConstant<int>(5, mKernalSizeY);
	Shader downsampleShader = Shader(gContext, "assets/shaders/bloom/downsample.comp");
	downsampleShader.SetSpecializationConstant<int>(0, mKernalSizeX);
	downsampleShader.SetSpecializationConstant<int>(1, mKernalSizeY);

	mSampler = vk::Gfx_CreateSampler(gContext,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		0.0,
		VK_TRUE,
		16.0,
		0.0,
		1000.0,
		VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

	BindingDescription thresholdBindings[2];
	thresholdBindings[0].mBindingID = 0;
	thresholdBindings[0].mFlags = 0;
	thresholdBindings[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	thresholdBindings[0].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	thresholdBindings[0].mTextures.push_back(fbo.m_color_attachments[colorAttachmentIndex].GetAttachment());
	thresholdBindings[0].mSampler = mSampler;

	thresholdBindings[1].mBindingID = 1;
	thresholdBindings[1].mFlags = 0;
	thresholdBindings[1].mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	thresholdBindings[1].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	thresholdBindings[1].mTextures.push_back(mDownsampleTexture);
	thresholdBindings[1].mSampler = mSampler;
	
	BindingDescription downsampleBindings[2];
	downsampleBindings[0].mBindingID = 0;
	downsampleBindings[0].mFlags = 0;
	downsampleBindings[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	downsampleBindings[0].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	downsampleBindings[0].mTextures.push_back(mDownsampleTexture);
	downsampleBindings[0].mSampler = mSampler;
	downsampleBindings[0].mCustomImageLayouts.insert(std::make_pair(0u, VK_IMAGE_LAYOUT_GENERAL));

	downsampleBindings[1].mBindingID = 1;
	downsampleBindings[1].mFlags = 0;
	downsampleBindings[1].mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	downsampleBindings[1].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	downsampleBindings[1].mTextures.push_back(mDownsampleTexture);
	downsampleBindings[1].mSampler = mSampler;

	std::vector<VkDescriptorPoolSize> poolSizes;
	ShaderConnector_CalculateDescriptorPool(2, thresholdBindings, poolSizes);
	ShaderConnector_CalculateDescriptorPool(2, downsampleBindings, poolSizes);
	mPool = vk::Gfx_CreateDescriptorPool(gContext, gFrameOverlapCount * 2, poolSizes);
	mThresholdSet = ShaderConnector_CreateSet(0, mPool, 2, thresholdBindings, 0, nullptr);
	mThresholdLayout = ShaderConnector_CreatePipelineLayout(1, &mThresholdSet, {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float)}});
	mThreshold = Pipeline_CreateCompute(gContext, &thresholdShader, mThresholdLayout, 0);
	mDownsampleSet = ShaderConnector_CreateSet(0, mPool, 2, downsampleBindings, 0, nullptr);
	mDownsampleLayout = ShaderConnector_CreatePipelineLayout(1, &mDownsampleSet, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DownsamplePushblock)} });
	mDownsample = Pipeline_CreateCompute(gContext, &downsampleShader, mDownsampleLayout, 0);

	mQuery = vk::Gfx_CreateQueryPool(gContext, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);

	auto cmd = vk::Gfx_CreateSingleUseCmdBuffer(gContext);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		Framebuffer_TransistionImage(cmd.cmd, mDownsampleTexture, VK_IMAGE_ASPECT_COLOR_BIT, i, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);
		RecordCommands(i);
	}
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(cmd);
}

Application::BloomPass::~BloomPass() {
	Super_Scene_Destroy();
	vkDestroySampler(mDevice, mSampler, nullptr);
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	vkDestroyPipelineLayout(mDevice, mThresholdLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mDownsampleLayout, nullptr);
	vkDestroyPipeline(mDevice, mThreshold, nullptr);
	vkDestroyPipeline(mDevice, mDownsample, nullptr);
	vkDestroyQueryPool(mDevice, mQuery, nullptr);
	Texture2_Destroy(mDownsampleTexture);
	Texture2_Destroy(mUpsampleTexture);
	ShaderConnector_DestroySet(mThresholdSet);
	ShaderConnector_DestroySet(mDownsampleSet);
}

void Application::BloomPass::ReloadShaders() {
	Shader thresholdShader = Shader(gContext, "assets/shaders/bloom/threshold.comp");
	thresholdShader.SetSpecializationConstant<int>(0, mFBO.m_width);
	thresholdShader.SetSpecializationConstant<int>(1, mFBO.m_height);
	thresholdShader.SetSpecializationConstant<int>(2, mTextureWidth);
	thresholdShader.SetSpecializationConstant<int>(3, mTextureHeight);
	thresholdShader.SetSpecializationConstant<int>(4, mKernalSizeX);
	thresholdShader.SetSpecializationConstant<int>(5, mKernalSizeY);
	Shader downsampleShader = Shader(gContext, "assets/shaders/bloom/downsample.comp");
	downsampleShader.SetSpecializationConstant<int>(0, mKernalSizeX);
	downsampleShader.SetSpecializationConstant<int>(1, mKernalSizeY);
	vkDestroyPipeline(mDevice, mThreshold, nullptr);
	vkDestroyPipeline(mDevice, mDownsample, nullptr);
	mThreshold = Pipeline_CreateCompute(gContext, &thresholdShader, mThresholdLayout, 0);
	mDownsample = Pipeline_CreateCompute(gContext, &downsampleShader, mDownsampleLayout, 0);

	for (int i = 0; i < gFrameOverlapCount; i++) {
		RecordCommands(i);
	}
}

VkCommandBuffer  Application::BloomPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	return mCmds[FrameIndex];
}

void Application::BloomPass::GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime) {
	uint64_t data[2];
	VkResult q = vkGetQueryPoolResults(mDevice, mQuery, FrameIndex * 2, 2, sizeof(data), data, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (Wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	if (q == VK_SUCCESS) {
		dTime = (double(data[1] - data[0]) * gContext->card.deviceLimits.timestampPeriod) * 1e-6;
	}
}

void Application::BloomPass::RecordCommands(uint32_t FrameIndex) {
	vkResetCommandPool(mDevice, mPools[FrameIndex], 0);
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkCommandBuffer cmd = mCmds[FrameIndex];
	vkBeginCommandBuffer(cmd, &beginInfo);
	vkCmdResetQueryPool(cmd, mQuery, (FrameIndex * 2), 2);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQuery, (FrameIndex * 2));

	// 1) Threshold Compute
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mThreshold);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mThresholdLayout, 0, 1, &mThresholdSet[FrameIndex], 0, nullptr);
	vkCmdPushConstants(cmd, mThresholdLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &mThresholdValue);
	vkCmdDispatch(cmd, (mTextureWidth + (mKernalSizeX - 1)) / mKernalSizeX, (mTextureHeight + (mKernalSizeY - 1)) / mKernalSizeY, 1);


	// 2) Downsample Compute
	uint32_t width = mDownsampleTexture->m_specification.m_Width;
	uint32_t height = mDownsampleTexture->m_specification.m_Height;
	DownsamplePushblock pushblock;
	pushblock.u_SourceMiplevel = 0;
	pushblock.u_SourceMipSize = glm::vec2(width, height);
	pushblock.u_SourceTexelSize = 1.0f / pushblock.u_SourceMipSize;
	pushblock.u_DestMipSize = glm::vec2(width >> 1, height >> 1);
	pushblock.u_DestTexelSize = 1.0f / pushblock.u_DestMipSize;
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mDownsample);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mDownsampleLayout, 0, 1, &mDownsampleSet[FrameIndex], 0, nullptr);
	for (uint32_t i = 0; i < mDownsampleTexture->mMipCount; i++) {
		pushblock.u_SourceMiplevel = i;
		pushblock.u_SourceMipSize = glm::vec2(width >> i, height >> i);
		pushblock.u_SourceTexelSize = 1.0f / pushblock.u_SourceMipSize;
		pushblock.u_DestMipSize = glm::vec2(width >> (1 + i), height >> (1 + i));
		pushblock.u_DestTexelSize = 1.0f / pushblock.u_DestMipSize;
		vkCmdPushConstants(cmd, mDownsampleLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushblock), &pushblock);
		uint32_t groupSizeX = (pushblock.u_DestMipSize[0] + (mKernalSizeX - 1)) / mKernalSizeX;
		uint32_t groupSizeY = (pushblock.u_DestMipSize[1] + (mKernalSizeY - 1)) / mKernalSizeY;
		VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = mDownsampleTexture->m_vk_images_per_frame[FrameIndex];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseMipLevel = i;
		barrier.subresourceRange.levelCount = 1;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
	}

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 1);
	vkEndCommandBuffer(cmd);
}
