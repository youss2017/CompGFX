#include "GeometryPass.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "../../window/PlatformWindow.hpp"
#include "perlin_noise.hpp"
#include <stb/stb_image_write.h>

extern vk::VkContext gContext;
namespace Application {
	extern PlatformWindow* gWindow;
}

struct TerrainPushblock {
	glm::mat4 u_Model;
	glm::mat3 u_NormalModel; // transpose(inverse(u_Model)) (no offsets)
};

Application::GeometryPass::GeometryPass(IBuffer2 verticesSSBO, IBuffer2 indicesSSBO, Terrain* terrain, int width, int height, FrustumCullPass* cullPass, Camera* camera, EntityController* ecs, ITexture2 shadowMap) : Scene(gContext->defaultDevice, true), mCamera(camera), mECS(ecs), mIndicsSSBO(indicesSSBO), mCullPass(cullPass) {
	
	mFBO.m_width = width;
	mFBO.m_height = height;
	FramebufferAttachment colorAttachment = FramebufferAttachment::Create(gContext, VK_IMAGE_USAGE_STORAGE_BIT, width, height, VK_FORMAT_B10G11R11_UFLOAT_PACK32, { 0.5, 0.5, 0.6, 0 });
	FramebufferAttachment depthAttachment = FramebufferAttachment::Create(gContext, 0, width, height, VK_FORMAT_D32_SFLOAT, { 1.0f });
	mFBO.AddColorAttachment(0, colorAttachment);
	mFBO.SetDepthAttachment(depthAttachment);

	/* Transition Framebuffer Textures into SHADER_READ_ONLY layout which is what the render pass is expecting */
	auto transitionCmd = vk::Gfx_CreateSingleUseCmdBuffer(gContext);
	Framebuffer_TransistionAttachment(transitionCmd.cmd, &colorAttachment, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED);
	Framebuffer_TransistionAttachment(transitionCmd.cmd, &depthAttachment, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED);
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(transitionCmd);
	
	mSampler = vk::Gfx_CreateSampler(gContext);
	mT0 = terrain;
	mShadowSampler = vk::Gfx_CreateSampler(gContext,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		0.0f, VK_TRUE, 16.0f, 0.01f, 1000.0f,
		VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
	mWoodTex = Texture2_CreateFromFile(gContext, "assets/textures/wood.png", true);
	mSandTex = Texture2_CreateFromFile(gContext, "assets/textures/sand.jpg", true);
	mStatueTex = Texture2_CreateFromFile(gContext, "assets/textures/statue.jpg", true);
	mNormalMap = Texture2_CreateFromFile(gContext, "assets/textures/normal.jpg", true);
	mLightDirection = glm::vec4(0.0);
	mQuery = vk::Gfx_CreateQueryPool(gContext, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);
	mInvocationQuery = vk::Gfx_CreateQueryPool(gContext, VK_QUERY_TYPE_PIPELINE_STATISTICS, gFrameOverlapCount, VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT);
	
	BindingDescription geometryPass[4];
	geometryPass[0].mBindingID = 0;
	geometryPass[0].mFlags = 0;
	geometryPass[0].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	geometryPass[0].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[0].mBufferSize = 0;
	geometryPass[0].mBuffer = verticesSSBO;
	geometryPass[0].mInternalSharedResources = true;

	geometryPass[1].mBindingID = 1;
	geometryPass[1].mFlags = BINDING_FLAG_CPU_VISIBLE;
	geometryPass[1].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	geometryPass[1].mStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPass[1].mBufferSize = sizeof(ShaderTypes::GlobalData);
	geometryPass[1].mBuffer = nullptr;

	geometryPass[2].mBindingID = 2;
	geometryPass[2].mFlags = 0;
	geometryPass[2].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	geometryPass[2].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[2].mBufferSize = 0;
	geometryPass[2].mBuffer = cullPass->mOutputGeometryDataArray;
	geometryPass[2].mInternalSharedResources = true;

	geometryPass[3].mBindingID = 3;
	geometryPass[3].mFlags = 0;
	geometryPass[3].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	geometryPass[3].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[3].mBufferSize = 0;
	geometryPass[3].mBuffer = cullPass->mOutputDrawDataArray;
	geometryPass[3].mInternalSharedResources = true;

	BindingDescription geometryPassFragment[2];
	geometryPassFragment[0].mBindingID = 0;
	geometryPassFragment[0].mFlags = 0;
	geometryPassFragment[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	geometryPassFragment[0].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPassFragment[0].mTextures.push_back(mWoodTex);
	geometryPassFragment[0].mSampler = mSampler;

	geometryPassFragment[1].mBindingID = 1;
	geometryPassFragment[1].mFlags = 0;
	geometryPassFragment[1].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	geometryPassFragment[1].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPassFragment[1].mTextures.push_back(shadowMap);
	geometryPassFragment[1].mSampler = mShadowSampler;

	BindingDescription terrainBindings[4];
	terrainBindings[0].mBindingID = 0;
	terrainBindings[0].mFlags = 0;
	terrainBindings[0].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	terrainBindings[0].mStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainBindings[0].mBufferSize = 0;
	terrainBindings[0].mBuffer = nullptr;
	terrainBindings[0].mInternalSharedResources = true;

	terrainBindings[1].mBindingID = 1;
	terrainBindings[1].mFlags = 0;
	terrainBindings[1].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	terrainBindings[1].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainBindings[1].mTextures.push_back(mSandTex);
	terrainBindings[1].mSampler = mSampler;

	terrainBindings[2].mBindingID = 2;
	terrainBindings[2].mFlags = 0;
	terrainBindings[2].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	terrainBindings[2].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainBindings[2].mTextures.push_back(shadowMap);
	terrainBindings[2].mSampler = mShadowSampler;

	terrainBindings[3].mBindingID = 3;
	terrainBindings[3].mFlags = 0;
	terrainBindings[3].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	terrainBindings[3].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainBindings[3].mTextures.push_back(mNormalMap);
	terrainBindings[3].mSampler = mSampler;

	std::vector<VkDescriptorPoolSize> poolSizes;
	ShaderConnector_CalculateDescriptorPool(4, geometryPass, poolSizes);
	ShaderConnector_CalculateDescriptorPool(2, geometryPassFragment, poolSizes);
	ShaderConnector_CalculateDescriptorPool(4, terrainBindings, poolSizes);
	mPool = vk::Gfx_CreateDescriptorPool(gContext, gFrameOverlapCount * 2 + (gFrameOverlapCount * 1), poolSizes);
	
	mGeoSet0 = ShaderConnector_CreateSet(0, mPool, 4, geometryPass, 0, nullptr);
	mGeoSet1 = ShaderConnector_CreateSet(1, mPool, 2, geometryPassFragment, 0, nullptr);
	DescriptorSet GeoSets[2] = { mGeoSet0, mGeoSet1 };
	mGeoLayout = ShaderConnector_CreatePipelineLayout(2, GeoSets, {});
	Shader geoVertex = Shader(gContext, "assets/shaders/vertex.vert");
	Shader geoFragment = Shader(gContext, "assets/shaders/fragment.frag");
	PipelineSpecification spec;
	spec.m_CullMode = CullMode::CULL_BACK;
	spec.m_DepthEnabled = true;
	spec.m_DepthWriteEnable = true;
	spec.m_DepthFunc = DepthFunction::LESS;
	spec.m_PolygonMode = PolygonMode::FILL;
	spec.m_FrontFaceCCW = true;
	spec.m_Topology = PolygonTopology::TRIANGLE_LIST;
	spec.m_SampleRateShadingEnabled = false;
	spec.m_MinSampleShading = 0.0;
	spec.m_NearField = 0.0f;
	spec.m_FarField = 1.0f;
	PipelineVertexInputDescription input;
	mGeoState = PipelineState_Create(gContext, spec, input, mFBO, mGeoLayout, &geoVertex, &geoFragment);

	terrainBindings[0].mBuffer = mGeoSet0.GetBuffer2(1);
	
	mMapSet = ShaderConnector_CreateSet(0, mPool, 4, terrainBindings, 0, nullptr);
	mMapLayout = ShaderConnector_CreatePipelineLayout(1, &mMapSet, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TerrainPushblock)} });
	Shader mapVertex = Shader(gContext, "assets/shaders/Terrain.vert");
	Shader mapFragment = Shader(gContext, "assets/shaders/Terrain.frag");
	input = {};
	input.AddInputElement("inPosition", 0, 0, 3, true, false, false);
	input.AddInputElement("inNormal", 1, 0, 3, true, false, false);
	input.AddInputElement("inTangent", 2, 0, 3, true, false, false);
	input.AddInputElement("inBiTangent", 3, 0, 3, true, false, false);
	input.AddInputElement("inTextureIDs", 4, 0, 3, false, false, false);
	input.AddInputElement("inTextureWeights", 5, 0, 3, true, false, false);
	input.AddInputElement("inTexCoords", 6, 0, 2, true, false, false);
	mMapState = PipelineState_Create(gContext, mGeoState->m_spec, input, mFBO, mMapLayout, &mapVertex, &mapFragment);

	for (int i = 0; i < gFrameOverlapCount; i++) {
		RecordCommands(i);
	}

}

Application::GeometryPass::~GeometryPass() {
	Super_Scene_Destroy();
	mFBO.DestroyAllBoundAttachments();
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	ShaderConnector_DestroySet(mGeoSet0);
	ShaderConnector_DestroySet(mGeoSet1);
	ShaderConnector_DestroySet(mMapSet);
	vkDestroyPipelineLayout(mDevice, mGeoLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mMapLayout, nullptr);
	PipelineState_Destroy(mGeoState);
	PipelineState_Destroy(mMapState);
	vkDestroySampler(mDevice, mSampler, nullptr);
	vkDestroySampler(mDevice, mShadowSampler, nullptr);
	vkDestroyQueryPool(mDevice, mQuery, nullptr);
	vkDestroyQueryPool(mDevice, mInvocationQuery, nullptr);
	Texture2_Destroy(mWoodTex);
	Texture2_Destroy(mStatueTex);
	Texture2_Destroy(mNormalMap);
	Texture2_Destroy(mSandTex);
}

VkCommandBuffer Application::GeometryPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	mECS->PrepareDataForFrame(FrameIndex);

	glm::mat4 proj = glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->GetWidth(), (float)gWindow->GetHeight(), 0.1f, 1000.0f);

	auto globalDataBuffer = mGeoSet0.GetBuffer2(1);
	void* ptr = Buffer2_Map(globalDataBuffer);
	ShaderTypes::GlobalData data;
	glm::mat4 view = mCamera->GetViewMatrix();
	data.u_LightDirection = mLightDirection;

	data.u_DeltaTime = dTime;
	data.u_TimeFromStart = dTimeFromStart;
	data.u_View = view;
	data.u_Projection = proj;
	data.u_ProjView = proj * view;
	data.u_LightSpace = mLightSpace;
	memcpy(ptr, &data, sizeof(data));
	Buffer2_Flush(globalDataBuffer, 0, VK_WHOLE_SIZE);
	return mCmds[FrameIndex];
}

void Application::GeometryPass::ReloadShaders() {
	PipelineSpecification specification = mGeoState->m_spec;
	PipelineState_Destroy(mGeoState);
	PipelineState_Destroy(mMapState);
	Shader geoVertex = Shader(gContext, "assets/shaders/vertex.vert");
	Shader geoFragment = Shader(gContext, "assets/shaders/fragment.frag");
	Shader mapVertex = Shader(gContext, "assets/shaders/Terrain.vert");
	Shader mapFragment = Shader(gContext, "assets/shaders/Terrain.frag");
	PipelineVertexInputDescription input;
	mGeoState = PipelineState_Create(gContext, specification, input, mFBO, mGeoLayout, &geoVertex, &geoFragment);
	input.AddInputElement("inPosition", 0, 0, 3, true, false, false);
	input.AddInputElement("inNormal", 1, 0, 3, true, false, false);
	input.AddInputElement("inTangent", 2, 0, 3, true, false, false);
	input.AddInputElement("inBiTangent", 3, 0, 3, true, false, false);
	input.AddInputElement("inTextureIDs", 4, 0, 3, false, false, false);
	input.AddInputElement("inTextureWeights", 5, 0, 3, true, false, false);
	input.AddInputElement("inTexCoords", 6, 0, 2, true, false, false);
	mMapState = PipelineState_Create(gContext, specification, input, mFBO, mMapLayout, &mapVertex, &mapFragment);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		RecordCommands(i);
	}
}

void Application::GeometryPass::SetWireframeMode(bool mode)
{
	if (mWireframeMode == mode)
		return;
	mWireframeMode = mode;
	PipelineSpecification specification = mGeoState->m_spec;
	specification.m_PolygonMode = mode ? PolygonMode::LINE : PolygonMode::FILL;
	PipelineState_Destroy(mGeoState);
	Shader geoVertex = Shader(gContext, "assets/shaders/vertex.vert");
	Shader geoFragment = Shader(gContext, "assets/shaders/fragment.frag");
	PipelineVertexInputDescription input;
	mGeoState = PipelineState_Create(gContext, specification, input, mFBO, mGeoLayout, &geoVertex, &geoFragment);
	PipelineState_Destroy(mMapState);
	Shader mapVertex = Shader(gContext, "assets/shaders/Terrain.vert");
	Shader mapFragment = Shader(gContext, "assets/shaders/Terrain.frag");
	input.AddInputElement("inPosition", 0, 0, 3, true, false, false);
	input.AddInputElement("inNormal", 1, 0, 3, true, false, false);
	input.AddInputElement("inTangent", 2, 0, 3, true, false, false);
	input.AddInputElement("inBiTangent", 3, 0, 3, true, false, false);
	input.AddInputElement("inTextureIDs", 4, 0, 3, false, false, false);
	input.AddInputElement("inTextureWeights", 5, 0, 3, true, false, false);
	input.AddInputElement("inTexCoords", 6, 0, 2, true, false, false);
	mMapState = PipelineState_Create(gContext, specification, input, mFBO, mMapLayout, &mapVertex, &mapFragment);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		this->RecordCommands(i);
	}
}

void Application::GeometryPass::GetStatistics(bool wait, uint32_t frameIndex, double& passTime, uint64_t& vertexInvocations, uint64_t& fragmentInvocations)
{
	uint64_t data0[2];
	uint64_t invocs[2]{};
	VkResult r0 = vkGetQueryPoolResults(mDevice, mQuery, frameIndex * 2, 2, sizeof(data0), data0, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	VkResult r1 = vkGetQueryPoolResults(mDevice, mInvocationQuery, frameIndex, 1, sizeof(invocs), invocs, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	if (r0 == VK_SUCCESS && r1 == VK_SUCCESS) {
		passTime = (double(data0[1] - data0[0]) * gContext->card.deviceLimits.timestampPeriod) * 1e-6;
		vertexInvocations = invocs[0];
		fragmentInvocations = invocs[1];
	}
}

void Application::GeometryPass::RecordCommands(uint32_t FrameIndex)
{
	uint32_t i = FrameIndex;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkCommandBuffer cmd = mCmds[i];
	vkResetCommandPool(mDevice, mPools[i], 0);
	vkBeginCommandBuffer(cmd, &beginInfo);
	vkCmdResetQueryPool(cmd, mQuery, FrameIndex * 2, 2);
	vkCmdResetQueryPool(cmd, mInvocationQuery, FrameIndex, 1);

	VkRenderingInfo renderingInfo;
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = nullptr;
	renderingInfo.flags = VK_RENDERING_SUSPENDING_BIT;
	renderingInfo.renderArea = { {0, 0}, { mFBO.m_width, mFBO.m_height } };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;

	VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	colorAttachment.imageView = mFBO.m_color_attachments[0].GetAttachment()->m_vk_views_per_frame[i];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	colorAttachment.resolveImageView = nullptr;
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = mFBO.m_color_attachments[0].mClear;
	VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthAttachment.imageView = mFBO.m_depth_attachment->GetAttachment()->m_vk_views_per_frame[i];
	depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	depthAttachment.resolveImageView = nullptr;
	depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.clearValue = mFBO.m_depth_attachment.value().mClear;

	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthAttachment;
	renderingInfo.pStencilAttachment = nullptr;
	vkCmdBeginRenderingKHR(cmd, &renderingInfo);

	VkImageMemoryBarrier renderBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	renderBarrier.srcAccessMask = VK_ACCESS_NONE;
	renderBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	renderBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	renderBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	renderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	renderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	renderBarrier.image = mFBO.m_color_attachments[0].GetAttachment()->m_vk_images_per_frame[i];
	renderBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	renderBarrier.subresourceRange.layerCount = 1;
	renderBarrier.subresourceRange.levelCount = 1;

	VkImageMemoryBarrier depthBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	depthBarrier.srcAccessMask = VK_ACCESS_NONE;
	depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depthBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	depthBarrier.image = mFBO.m_depth_attachment->GetAttachment()->m_vk_images_per_frame[i];
	depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthBarrier.subresourceRange.layerCount = 1;
	depthBarrier.subresourceRange.levelCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &renderBarrier);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthBarrier);

	vkCmdBeginQuery(cmd, mInvocationQuery, FrameIndex, 0);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 0);

	VkDescriptorSet geometrySets[2] = { mGeoSet0[i], mGeoSet1[i] };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGeoState->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGeoLayout, 0, 2, geometrySets, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, mIndicsSSBO->mBuffers[FrameIndex], 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirect(cmd, mCullPass->mOutputDrawDataArray->mBuffers[FrameIndex], 0, mECS->GetDrawCount(), sizeof(ShaderTypes::DrawData));

	TerrainPushblock pushblock;
	pushblock.u_Model = glm::scale(glm::mat4(1.0), glm::vec3(2.0));
	pushblock.u_NormalModel = glm::mat3(glm::transpose(glm::inverse(pushblock.u_Model)));

	VkDescriptorSet mapSet[1] = { mMapSet[FrameIndex] };
	VkDeviceSize offset[1] = { 0 };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mMapState->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mMapLayout, 0, 1, mapSet, 0, nullptr);
	vkCmdPushConstants(cmd, mMapLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TerrainPushblock), &pushblock);
	vkCmdBindVertexBuffers(cmd, 0, 1, &mT0->GetVerticesBuffer()->mBuffers[FrameIndex], offset);
	vkCmdBindIndexBuffer(cmd, mT0->GetIndicesBuffer()->mBuffers[FrameIndex], 0, VK_INDEX_TYPE_UINT32);
	for (uint32_t i = 0; i < mT0->GetSubmeshCount(); i++) {
		auto& submesh = mT0->GetSubmesh(i);
		vkCmdDrawIndexed(cmd, submesh.mIndicesCount, 1, submesh.mFirstIndex, submesh.mFirstVertex, 0);
	}

	VkImageMemoryBarrier presentBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	presentBarrier.dstAccessMask = VK_ACCESS_NONE;
	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	presentBarrier.image = mFBO.m_color_attachments[0].GetImage(i);
	presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentBarrier.subresourceRange.layerCount = 1;
	presentBarrier.subresourceRange.levelCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 1);
	vkCmdEndQuery(cmd, mInvocationQuery, FrameIndex);

	vkCmdEndRenderingKHR(cmd);
	vkEndCommandBuffer(cmd);
}
