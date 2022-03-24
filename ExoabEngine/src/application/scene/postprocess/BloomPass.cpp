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

	mSampler = vk::Gfx_CreateSampler(gContext);

	std::vector<ShaderBinding> thresholdBindings(2);
	thresholdBindings[0].m_type = SHADER_BINDING_COMBINED_TEXTURE_SAMPLER;
	thresholdBindings[0].m_bindingID = 0;
	thresholdBindings[0].m_preinitalized = false;
	thresholdBindings[0].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	thresholdBindings[0].m_sampler.push_back(mSampler);
	thresholdBindings[0].m_textures.push_back(fbo.m_color_attachments[colorAttachmentIndex].GetAttachment());
	thresholdBindings[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	thresholdBindings[1].m_type = SHADER_BINDING_STORAGE_IMAGE;
	thresholdBindings[1].m_bindingID = 1;
	thresholdBindings[1].m_preinitalized = false;
	thresholdBindings[1].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	thresholdBindings[1].m_sampler.push_back(mSampler);
	thresholdBindings[1].m_textures.push_back(mDownsampleTexture);
	thresholdBindings[1].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_GENERAL);

	std::vector<ShaderBinding> downsampleBindings(1);
	downsampleBindings[0].m_type = SHADER_BINDING_STORAGE_IMAGE;
	downsampleBindings[0].m_bindingID = 0;
	downsampleBindings[0].m_preinitalized = false;
	downsampleBindings[0].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	downsampleBindings[0].m_sampler.push_back(mSampler);
	downsampleBindings[0].m_textures.push_back(mDownsampleTexture);
	downsampleBindings[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_GENERAL);
	downsampleBindings[0].mUseViewPerMip = true;

	std::vector<VkDescriptorPoolSize> poolSizes;
	ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSizes, &thresholdBindings);
	ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSizes, &downsampleBindings);
	mPool = vk::Gfx_CreateDescriptorPool(gContext, gFrameOverlapCount * 2, poolSizes);
	mThresholdSet = ShaderBinding_Create(gContext, mPool, 0, &thresholdBindings);
	mThresholdLayout = ShaderBinding_CreatePipelineLayout(gContext, { mThresholdSet }, {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float)}});
	mThreshold = Pipeline_CreateCompute(gContext, &thresholdShader, mThresholdLayout, 0);
	mDownsampleSet = ShaderBinding_Create(gContext, mPool, 0, &downsampleBindings);
	mDownsampleLayout = ShaderBinding_CreatePipelineLayout(gContext, { mDownsampleSet }, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DownsamplePushblock)} });
	mDownsample = Pipeline_CreateCompute(gContext, &downsampleShader, mDownsampleLayout, 0);

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
	Texture2_Destroy(mDownsampleTexture);
	Texture2_Destroy(mUpsampleTexture);
	ShaderBinding_DestroySets(gContext, { mThresholdSet, mDownsampleSet });
}

void Application::BloomPass::ReloadShaders() {

}

VkCommandBuffer  Application::BloomPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	return mCmds[FrameIndex];
}

void Application::BloomPass::RecordCommands(uint32_t FrameIndex) {
	vkResetCommandPool(mDevice, mPools[FrameIndex], 0);
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkCommandBuffer cmd = mCmds[FrameIndex];
	vkBeginCommandBuffer(cmd, &beginInfo);

	// 1) Threshold Compute
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mThreshold);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mThresholdLayout, 0, 1, &mThresholdSet->m_set[FrameIndex], 0, nullptr);
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
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mDownsampleLayout, 0, 1, &mDownsampleSet->m_set[FrameIndex], 0, nullptr);
	for (int i = 0; i < mDownsampleTexture->mMipCount; i++) {
		pushblock.u_SourceMiplevel = i;
		pushblock.u_SourceMipSize = glm::vec2(width >> i, height >> i);
		pushblock.u_SourceTexelSize = 1.0f / pushblock.u_SourceMipSize;
		pushblock.u_DestMipSize = glm::vec2(width >> (1 + i), height >> (1 + i));
		pushblock.u_DestTexelSize = 1.0f / pushblock.u_DestMipSize;
		vkCmdPushConstants(cmd, mDownsampleLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushblock), &pushblock);
		uint32_t groupSizeX = (pushblock.u_DestMipSize[0] + (mKernalSizeX - 1)) / mKernalSizeX;
		uint32_t groupSizeY = (pushblock.u_DestMipSize[1] + (mKernalSizeY - 1)) / mKernalSizeY;
		vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
	}

	vkEndCommandBuffer(cmd);
}
