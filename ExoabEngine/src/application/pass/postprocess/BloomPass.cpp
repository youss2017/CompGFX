#include "BloomPass.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <glm/glm.hpp>
#include "../../Globals.hpp"

using namespace glm;

#define HORIZONTAL_BLUR 0
#define VERTICAL_BLUR 1
#define BLOOM_FILTER 2
#define BLOOM_DOWNSAMPLE 3
#define BLOOM_UPSAMPLE 4

struct BloomSettingsPushblock {
	int Option;
	int LOD;
	float BloomFilterThreshold;
	vec3 BloomCurve;
	ivec2 Size;
	vec2 TexelSize;
	ivec2 DstSize;
	vec2 DstTexelSize;
};

Application::BloomPass::BloomPass(Framebuffer& fbo, int colorAttachmentIndex, float threshold) : Pass(Global::Context->defaultDevice, true), mFBO(fbo), mThresholdValue(threshold) {
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
	mBlurTexture = Texture2_Create(spec);
	Shader bloomShader = Shader("assets/shaders/postprocess/bloom/bloom.comp");
	bloomShader.SetSpecializationConstant<int>(0, 1);

	mSampler = vk::Gfx_CreateSampler(Global::Context,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	mSourceSize = ivec2(fbo.m_width, fbo.m_height);

	std::vector<VkDescriptorPoolSize> poolSizes;
	BindingDescription bloomDescription[3]{};
	bloomDescription[0].mBindingID = 0;
	bloomDescription[0].mFlags = 0;
	bloomDescription[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bloomDescription[0].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	bloomDescription[0].mTextures.push_back(fbo.m_color_attachments[colorAttachmentIndex].GetAttachment());
	bloomDescription[0].mSampler = mSampler;
	bloomDescription[0].mCustomImageLayouts.insert({ 0, VK_IMAGE_LAYOUT_GENERAL });

	bloomDescription[1].mBindingID = 1;
	bloomDescription[1].mFlags = 0;
	bloomDescription[1].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bloomDescription[1].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	bloomDescription[1].mTextures.push_back(mBlurTexture);
	bloomDescription[1].mSampler = mSampler;
	bloomDescription[1].mCustomImageLayouts.insert({ 0, VK_IMAGE_LAYOUT_GENERAL });

	bloomDescription[2].mBindingID = 2;
	bloomDescription[2].mFlags = 0;
	bloomDescription[2].mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bloomDescription[2].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	bloomDescription[2].mTextures.push_back(mBlurTexture);

	BindingDescription combineBindings[2]{};
	combineBindings[0].mBindingID = 0;
	combineBindings[0].mFlags = BINDING_FLAG_USING_ONLY_FIRST_MIP;
	combineBindings[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	combineBindings[0].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	combineBindings[0].mTextures.push_back(mBlurTexture);
	combineBindings[0].mSampler = mSampler;
	combineBindings[0].mCustomImageLayouts.insert({ 0, VK_IMAGE_LAYOUT_GENERAL });

	combineBindings[1].mBindingID = 1;
	combineBindings[1].mFlags = BINDING_FLAG_USING_ONLY_FIRST_MIP;
	combineBindings[1].mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	combineBindings[1].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	combineBindings[1].mTextures.push_back(fbo.m_color_attachments[colorAttachmentIndex].GetAttachment());
	combineBindings[1].mCustomImageLayouts.insert({ 0, VK_IMAGE_LAYOUT_GENERAL });

	ShaderConnector_CalculateDescriptorPool(3, bloomDescription, poolSizes);
	ShaderConnector_CalculateDescriptorPool(2, combineBindings, poolSizes);
	mPool = vk::Gfx_CreateDescriptorPool(Global::Context, gFrameOverlapCount * 2, poolSizes);

	mSet = ShaderConnector_CreateSet(0, mPool, 3, bloomDescription);
	mLayout = ShaderConnector_CreatePipelineLayout(1, &mSet, {{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomSettingsPushblock) }} );
	mPipeline = PipelineState_CreateCompute(&bloomShader, mLayout, 0);

	Shader combineShader = Shader("assets/shaders/postprocess/bloom/combine.comp");
	mCombineSet = ShaderConnector_CreateSet(0, mPool, 2, combineBindings);
	mCombineLayout = ShaderConnector_CreatePipelineLayout(1, &mCombineSet, {});
	mCombinePipeline = PipelineState_CreateCompute(&combineShader, mCombineLayout, 0);

	mQuery = vk::Gfx_CreateQueryPool(Global::Context, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);

	auto cmd = vk::Gfx_CreateSingleUseCmdBuffer(Global::Context);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		VkImageLayout dstLayout = VK_IMAGE_LAYOUT_GENERAL;
		vk::Framebuffer_TransistionImage(cmd.cmd, mBlurTexture, VK_IMAGE_ASPECT_COLOR_BIT, i, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, dstLayout);
		RecordCommands(i);
	}
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(cmd);
}

Application::BloomPass::~BloomPass() {
	Super_Pass_Destroy();
	Texture2_Destroy(mBlurTexture);
	ShaderConnector_DestroySet(mSet);
	ShaderConnector_DestroySet(mCombineSet);
	vkDestroySampler(mDevice, mSampler, nullptr);
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	vkDestroyPipeline(Global::Context->defaultDevice, mPipeline, nullptr);
	vkDestroyPipelineLayout(Global::Context->defaultDevice, mLayout, nullptr);
	vkDestroyPipeline(Global::Context->defaultDevice, mCombinePipeline, nullptr);
	vkDestroyPipelineLayout(Global::Context->defaultDevice, mCombineLayout, nullptr);
	vkDestroyQueryPool(Global::Context->defaultDevice, mQuery, nullptr);
}

void Application::BloomPass::ReloadShaders() {
	Shader bloomShader = Shader("assets/shaders/postprocess/bloom/bloom.comp");
	bloomShader.SetSpecializationConstant<int>(0, true);
	vkDestroyPipeline(Global::Context->defaultDevice, mPipeline, nullptr);
	mPipeline = PipelineState_CreateCompute(&bloomShader, mLayout, 0);
	Shader combineShader = Shader("assets/shaders/postprocess/bloom/combine.comp");
	vkDestroyPipeline(Global::Context->defaultDevice, mCombinePipeline, nullptr);
	mCombinePipeline = PipelineState_CreateCompute(&combineShader, mCombineLayout, 0);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		RecordCommands(i);
	}
}

VkCommandBuffer  Application::BloomPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
#pragma warning("REMOVE THIS IN BloomPass.cpp")
	RecordCommands(FrameIndex);
	return *mCmd;
}

void Application::BloomPass::GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime) {
	uint64_t data[2];
	VkResult q = vkGetQueryPoolResults(mDevice, mQuery, FrameIndex * 2, 2, sizeof(data), data, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (Wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	if (q == VK_SUCCESS) {
		dTime = (double(data[1] - data[0]) * Global::Context->card.deviceLimits.timestampPeriod) * 1e-6;
	}
}

void Application::BloomPass::RecordCommands(uint32_t FrameIndex) {
	mCmdPool->Reset(FrameIndex);
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkCommandBuffer cmd = mCmd->mCmds[FrameIndex];
	vkBeginCommandBuffer(cmd, &beginInfo);
	vk::Gfx_InsertDebugLabel(cmd, FrameIndex, "[PostProcess] Bloom Pass", 1.0);

	vkCmdResetQueryPool(cmd, mQuery, (FrameIndex * 2), 2);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQuery, (FrameIndex * 2));

	
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0, 1, &mSet[FrameIndex], 0, nullptr);

	const int localSizeX = 16;
	const int localSizeY = 16;

	// 1) Filter Pass
	vk::Gfx_InsertDebugLabel(cmd, FrameIndex, "Prefilter", 0.0, 1.0);
	BloomSettingsPushblock pushblock{};
	pushblock.Option = BLOOM_FILTER;
	pushblock.Size = mSourceSize;
	pushblock.TexelSize = 1.0f / vec2(mSourceSize);
	pushblock.DstSize = ivec2(mBlurTexture->m_specification.m_Width, mBlurTexture->m_specification.m_Height);
	pushblock.DstTexelSize = 1.0f / vec2(pushblock.DstSize);
	pushblock.BloomFilterThreshold = 0.5;
	pushblock.BloomCurve = vec3(0.9, 0.2, 1.5);

	int groupSizeX = (pushblock.DstSize[0] + (localSizeX - 1)) / localSizeX;
	int groupSizeY = (pushblock.DstSize[1] + (localSizeY - 1)) / localSizeY;

	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomSettingsPushblock), &pushblock);
	vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
	DvkCmdEndDebugUtilsLabelEXT(cmd);

	auto barrier = [FrameIndex, &cmd, this](VkImage image = nullptr) throw() -> void {
		VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image ? image : mBlurTexture->m_vk_image->m_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.layerCount= VK_REMAINING_ARRAY_LAYERS;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	};

	// 2) Downsample Blur
	vk::Gfx_InsertDebugLabel(cmd, FrameIndex, "Downsample Blur", 1.0, 0.0);
	for (uint32_t i = 0; i < mBlurTexture->mMipCount - 1; i++) {
		barrier();
		BloomSettingsPushblock pushblock{};
		pushblock.Option = BLOOM_DOWNSAMPLE;
		pushblock.Size = ivec2(mBlurTexture->m_specification.m_Width >> i, mBlurTexture->m_specification.m_Height >> i);
		pushblock.TexelSize = 1.0f / vec2(pushblock.Size);
		pushblock.DstSize = ivec2(mBlurTexture->m_specification.m_Width >> (i + 1), mBlurTexture->m_specification.m_Height >> (i + 1));
		pushblock.DstTexelSize = 1.0f / vec2(pushblock.DstSize);
		pushblock.BloomFilterThreshold = 0.5;
		pushblock.BloomCurve = vec3(0.9, 0.2, 1.5);
		pushblock.LOD = i;

		int groupSizeX = (pushblock.DstSize[0] + (localSizeX - 1)) / localSizeX;
		int groupSizeY = (pushblock.DstSize[1] + (localSizeY - 1)) / localSizeY;

		vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomSettingsPushblock), &pushblock);
		vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
	}
	DvkCmdEndDebugUtilsLabelEXT(cmd);
	vk::Gfx_InsertDebugLabel(cmd, FrameIndex, "Upsample", 1.0, 0.0);
	// 3) Upsample
	for(uint32_t i = 1; i < mBlurTexture->mMipCount - 1; i++) {
		barrier();
		BloomSettingsPushblock pushblock{};
		pushblock.Option = BLOOM_UPSAMPLE;
		pushblock.Size = ivec2(mBlurTexture->m_specification.m_Width, mBlurTexture->m_specification.m_Height);
		pushblock.TexelSize = 1.0f / vec2(pushblock.Size);
		pushblock.DstSize = ivec2(mBlurTexture->m_specification.m_Width >> i, mBlurTexture->m_specification.m_Height >> i);
		pushblock.DstTexelSize = 1.0f / vec2(pushblock.DstSize);
		pushblock.BloomFilterThreshold = 0.5;
		pushblock.BloomCurve = vec3(0.9, 0.2, 1.5);
		pushblock.LOD = i;

		int groupSizeX = (pushblock.Size[0] + (localSizeX - 1)) / localSizeX;
		int groupSizeY = (pushblock.Size[1] + (localSizeY - 1)) / localSizeY;

		vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomSettingsPushblock), &pushblock);
		vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
	}
	DvkCmdEndDebugUtilsLabelEXT(cmd);

	// 4) Combine
#if 0
	vk::Gfx_InsertDebugLabel(cmd, FrameIndex, "Apply Bloom", 0.0, 1.0);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mCombinePipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mCombineLayout, 0, 1, &mCombineSet[FrameIndex], 0, nullptr);
	vkCmdDispatch(cmd, (mSourceSize[0] + 31) / 32, (mSourceSize[1] + 31) / 32, 1);
	DvkCmdEndDebugUtilsLabelEXT(cmd);
#endif

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 1);
	DvkCmdEndDebugUtilsLabelEXT(cmd);
	vkEndCommandBuffer(cmd);
}
