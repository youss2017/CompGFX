#include "BloomPass.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <glm/glm.hpp>
#include "../../Globals.hpp"

using namespace glm;

#define HORIZONTAL_BLUR 0
#define VERTICAL_BLUR 1
#define BLOOM_FILTER 2

struct BloomSettingsPushblock {
	int Option;
	ivec2 Size;
	vec2 TexelSize;
	ivec2 DstSize;
	vec2 DstTexelSize;
	float BloomFilterThreshold;
};

Application::BloomPass::BloomPass(Framebuffer& fbo, int colorAttachmentIndex, float threshold) : Pass(Global::Context->defaultDevice, true), mFBO(fbo), mThresholdValue(threshold) {
	Texture2DSpecification spec;
	spec.m_Width = fbo.m_width;
	spec.m_Height = fbo.m_height;
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

	mSampler = vk::Gfx_CreateSampler(Global::Context);

	std::vector<VkDescriptorPoolSize> poolSizes;
	BindingDescription bloomDescription[2]{};
	bloomDescription[0].mBindingID = 0;
	bloomDescription[0].mFlags = 0;
	bloomDescription[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bloomDescription[0].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	bloomDescription[0].mTextures.push_back(fbo.m_color_attachments[colorAttachmentIndex].GetAttachment());
	bloomDescription[0].mSampler = mSampler;

	bloomDescription[1].mBindingID = 1;
	bloomDescription[1].mFlags = 0;
	bloomDescription[1].mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bloomDescription[1].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	bloomDescription[1].mTextures.push_back(mBlurTexture);

	ShaderConnector_CalculateDescriptorPool(2, bloomDescription, poolSizes);
	mPool = vk::Gfx_CreateDescriptorPool(Global::Context, gFrameOverlapCount, poolSizes);

	mSet = ShaderConnector_CreateSet(0, mPool, 2, bloomDescription);
	mLayout = ShaderConnector_CreatePipelineLayout(1, &mSet, {{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomSettingsPushblock) }} );
	mPipeline = PipelineState_CreateCompute(Global::Context, &bloomShader, mLayout, 0);

	mQuery = vk::Gfx_CreateQueryPool(Global::Context, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);

	auto cmd = vk::Gfx_CreateSingleUseCmdBuffer(Global::Context);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		vk::Framebuffer_TransistionImage(cmd.cmd, mBlurTexture, VK_IMAGE_ASPECT_COLOR_BIT, i, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);
		RecordCommands(i);
	}
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(cmd);
}

Application::BloomPass::~BloomPass() {
	Super_Pass_Destroy();
	Texture2_Destroy(mBlurTexture);
	ShaderConnector_DestroySet(mSet);
	vkDestroySampler(mDevice, mSampler, nullptr);
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	vkDestroyPipeline(Global::Context->defaultDevice, mPipeline, nullptr);
	vkDestroyPipelineLayout(Global::Context->defaultDevice, mLayout, nullptr);
	vkDestroyQueryPool(Global::Context->defaultDevice, mQuery, nullptr);
}

void Application::BloomPass::ReloadShaders() {
	Shader bloomShader = Shader("assets/shaders/postprocess/bloom/bloom.comp");
	vkDestroyPipeline(Global::Context->defaultDevice, mPipeline, nullptr);
	mPipeline = PipelineState_CreateCompute(Global::Context, &bloomShader, mLayout, 0);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		RecordCommands(i);
	}
}

VkCommandBuffer  Application::BloomPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
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
	vkCmdResetQueryPool(cmd, mQuery, (FrameIndex * 2), 2);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQuery, (FrameIndex * 2));

	BloomSettingsPushblock pushblock{};
	pushblock.Option = HORIZONTAL_BLUR;
	pushblock.Size = ivec2(mBlurTexture->m_specification.m_Width, mBlurTexture->m_specification.m_Height);
	pushblock.TexelSize = 1.0f / vec2(pushblock.Size);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0, 1, &mSet[FrameIndex], 0, nullptr);
	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomSettingsPushblock), &pushblock);

	const int localSizeX = 16;
	const int localSizeY = 16;

	int groupSizeX = (pushblock.Size[0] + (localSizeX - 1)) / localSizeX;
	int groupSizeY = (pushblock.Size[1] + (localSizeY - 1)) / localSizeY;

	vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);
	
	pushblock.Option = BLOOM_FILTER;
	pushblock.BloomFilterThreshold = 0.5;
	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomSettingsPushblock), &pushblock);
	vkCmdDispatch(cmd, groupSizeX, groupSizeY, 1);

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 1);
	vkEndCommandBuffer(cmd);
}
