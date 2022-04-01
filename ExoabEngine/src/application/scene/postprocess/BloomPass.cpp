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

struct DownsampleVerticalPushblock {
	glm::uint u_MipLevel;
	glm::vec2 u_MipSize;
	glm::vec2 u_MipTexelSize;
};

struct UpsamplePushblock {
	glm::uint u_MipLevel;
	glm::vec2 u_UpsampleMipSize;
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
	Shader downsampleVerticalShader = Shader(gContext, "assets/shaders/bloom/downsampleVertical.comp");
	downsampleVerticalShader.SetSpecializationConstant<int>(0, mKernalSizeX);
	downsampleVerticalShader.SetSpecializationConstant<int>(1, mKernalSizeY);
	Shader upsampleShader = Shader(gContext, "assets/shaders/bloom/upsample.comp");
	upsampleShader.SetSpecializationConstant<int>(0, mKernalSizeX);
	upsampleShader.SetSpecializationConstant<int>(1, mKernalSizeY);
	
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

	BindingDescription combineBindings[2];
	combineBindings[0].mBindingID = 0;
	combineBindings[0].mFlags = 0;
	combineBindings[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	combineBindings[0].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	combineBindings[0].mTextures.push_back(mFBO.m_color_attachments[colorAttachmentIndex].GetAttachment());
	combineBindings[0].mSampler = mSampler;
	combineBindings[0].mCustomImageLayouts.insert(std::make_pair(0u, VK_IMAGE_LAYOUT_GENERAL));

	combineBindings[1].mBindingID = 1;
	combineBindings[1].mFlags = 0;
	combineBindings[1].mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	combineBindings[1].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	combineBindings[1].mTextures.push_back(mDownsampleTexture);
	combineBindings[1].mSampler = mSampler;

	std::vector<VkDescriptorPoolSize> poolSizes;
	ShaderConnector_CalculateDescriptorPool(2, thresholdBindings, poolSizes);
	ShaderConnector_CalculateDescriptorPool(2, downsampleBindings, poolSizes);
	mPool = vk::Gfx_CreateDescriptorPool(gContext, gFrameOverlapCount * 2, poolSizes);
	mThresholdSet = ShaderConnector_CreateSet(0, mPool, 2, thresholdBindings);
	mThresholdLayout = ShaderConnector_CreatePipelineLayout(1, &mThresholdSet, {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float)}});
	mThreshold = Pipeline_CreateCompute(gContext, &thresholdShader, mThresholdLayout, 0);
	mDownsampleSet = ShaderConnector_CreateSet(0, mPool, 2, downsampleBindings);
	mDownsampleLayout = ShaderConnector_CreatePipelineLayout(1, &mDownsampleSet, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DownsamplePushblock)} });
	mDownsampleVerticalLayout = ShaderConnector_CreatePipelineLayout(1, &mDownsampleSet, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DownsampleVerticalPushblock)} });
	mUpsampleLayout = ShaderConnector_CreatePipelineLayout(1, &mDownsampleSet, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(UpsamplePushblock)} });
	mDownsample = Pipeline_CreateCompute(gContext, &downsampleShader, mDownsampleLayout, 0);
	mDownsampleVertical = Pipeline_CreateCompute(gContext, &downsampleVerticalShader, mDownsampleVerticalLayout, 0);
	mUpsample = Pipeline_CreateCompute(gContext, &upsampleShader, mUpsampleLayout, 0);

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
	vkDestroyPipelineLayout(mDevice, mDownsampleVerticalLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mUpsampleLayout, nullptr);
	vkDestroyPipeline(mDevice, mThreshold, nullptr);
	vkDestroyPipeline(mDevice, mDownsample, nullptr);
	vkDestroyPipeline(mDevice, mDownsampleVertical, nullptr);
	vkDestroyPipeline(mDevice, mUpsample, nullptr);
	vkDestroyQueryPool(mDevice, mQuery, nullptr);
	Texture2_Destroy(mDownsampleTexture);
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
	Shader downsampleVerticalShader = Shader(gContext, "assets/shaders/bloom/downsampleVertical.comp");
	downsampleVerticalShader.SetSpecializationConstant<int>(0, mKernalSizeX);
	downsampleVerticalShader.SetSpecializationConstant<int>(1, mKernalSizeY);
	Shader upsampleShader = Shader(gContext, "assets/shaders/bloom/upsample.comp");
	upsampleShader.SetSpecializationConstant<int>(0, mKernalSizeX);
	upsampleShader.SetSpecializationConstant<int>(1, mKernalSizeY);
	vkDestroyPipeline(mDevice, mThreshold, nullptr);
	vkDestroyPipeline(mDevice, mDownsample, nullptr);
	vkDestroyPipeline(mDevice, mDownsampleVertical, nullptr);
	vkDestroyPipeline(mDevice, mUpsample, nullptr);
	mThreshold = Pipeline_CreateCompute(gContext, &thresholdShader, mThresholdLayout, 0);
	mDownsample = Pipeline_CreateCompute(gContext, &downsampleShader, mDownsampleLayout, 0);
	mDownsampleVertical = Pipeline_CreateCompute(gContext, &downsampleVerticalShader, mDownsampleVerticalLayout, 0);
	mUpsample = Pipeline_CreateCompute(gContext, &upsampleShader, mUpsampleLayout, 0);

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

	auto FullBarrier = [&]() {
		VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = mDownsampleTexture->m_vk_images_per_frame[FrameIndex];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	};

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
		uint32_t groupSizeX = (pushblock.u_DestMipSize[0] + (mKernalSizeX - 1)) / mKernalSizeX;
		uint32_t groupSizeY = (pushblock.u_DestMipSize[1] + (mKernalSizeY - 1)) / mKernalSizeY;
		vkCmdPushConstants(cmd, mDownsampleLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushblock), &pushblock);
		vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
		FullBarrier();
	}

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mDownsampleVertical);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mDownsampleVerticalLayout, 0, 1, &mDownsampleSet[FrameIndex], 0, nullptr);
	for (uint32_t i = 0; i < mDownsampleTexture->mMipCount; i++) {
		DownsampleVerticalPushblock verticalPushblock;
		verticalPushblock.u_MipLevel = i - 1;
		verticalPushblock.u_MipSize = glm::vec2(width >> (i - 1), height >> (i - 1));
		verticalPushblock.u_MipTexelSize = 1.0f / verticalPushblock.u_MipSize;
		vkCmdPushConstants(cmd, mDownsampleVerticalLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(verticalPushblock), &verticalPushblock);
		uint32_t groupSizeX = (verticalPushblock.u_MipSize[0] + (mKernalSizeX - 1)) / mKernalSizeX;
		uint32_t groupSizeY = (verticalPushblock.u_MipSize[1] + (mKernalSizeY - 1)) / mKernalSizeY;
		vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
	}

	FullBarrier();

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mUpsample);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mUpsampleLayout, 0, 1, &mDownsampleSet[FrameIndex], 0, nullptr);
	for (uint32_t i = mDownsampleTexture->mMipCount; i > 0; i--) {
		UpsamplePushblock verticalPushblock;
		verticalPushblock.u_MipLevel = i;
		verticalPushblock.u_UpsampleMipSize = glm::vec2(width >> (i ), height >> (i));
		uint32_t groupSizeX = (verticalPushblock.u_UpsampleMipSize[0] + (mKernalSizeX - 1)) / mKernalSizeX;
		uint32_t groupSizeY = (verticalPushblock.u_UpsampleMipSize[1] + (mKernalSizeY - 1)) / mKernalSizeY;
		vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
		vkCmdPushConstants(cmd, mDownsampleVerticalLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(verticalPushblock), &verticalPushblock);
		FullBarrier();
	}

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 1);
	vkEndCommandBuffer(cmd);
}
