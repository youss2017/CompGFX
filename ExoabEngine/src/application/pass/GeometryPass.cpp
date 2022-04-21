#include "GeometryPass.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "window/PlatformWindow.hpp"
#include "perlin_noise.hpp"
#include <stb_image_write.h>
#include "../Globals.hpp"
#include "../minimap.hpp"

struct TerrainPushblock {
	glm::mat4 u_Model;
	glm::mat3 u_NormalModel; // transpose(inverse(u_Model)) (no offsets)
};

Application::GeometryPass::GeometryPass(uint32_t lightCount, ShaderTypes::Light* lights, IBuffer2 verticesSSBO, IBuffer2 indicesSSBO, Terrain* terrain, int width, int height, FrustumCullPass* cullPass, Camera* camera, Camera* lockedCamera, EntityController* ecs, ITexture2 shadowMap) : Pass(Global::Context->defaultDevice, true), mCamera(camera), mLockedCamera(lockedCamera), mECS(ecs), mIndicsSSBO(indicesSSBO), mCullPass(cullPass), mLightCount(lightCount), mLights(lights) {
	
	mTerrainDrawCommands = (VkDrawIndexedIndirectCommand*)Gmalloc(Global::Context, terrain->GetSubmeshCount() * sizeof(VkDrawIndexedIndirectCommand), BufferType::BUFFER_TYPE_INDIRECT, false);
	mTerrainDrawCount = (uint32_t*)Gmalloc(Global::Context, 4, BUFFER_TYPE_INDIRECT, true);

	mFBO.m_width = width;
	mFBO.m_height = height;
	FramebufferAttachment colorAttachment = FramebufferAttachment::Create(Global::Context, VK_IMAGE_USAGE_STORAGE_BIT, width, height, VK_FORMAT_B10G11R11_UFLOAT_PACK32, { 0.5, 0.5, 0.6, 0 });
	FramebufferAttachment depthAttachment = FramebufferAttachment::Create(Global::Context, 0, width, height, VK_FORMAT_D32_SFLOAT, { 1.0f });
	mFBO.AddColorAttachment(0, colorAttachment);
	mFBO.SetDepthAttachment(depthAttachment);

	/* Transition Framebuffer Textures into SHADER_READ_ONLY layout which is what the render pass is expecting */
	auto transitionCmd = vk::Gfx_CreateSingleUseCmdBuffer(Global::Context);
	vk::Framebuffer_TransistionAttachment(transitionCmd.cmd, &colorAttachment, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED);
	vk::Framebuffer_TransistionAttachment(transitionCmd.cmd, &depthAttachment, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED);
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(transitionCmd);
	
	mT0 = terrain;
	mShadowSampler = vk::Gfx_CreateSampler(Global::Context,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		0.0f, VK_TRUE, 16.0f, 0.01f, 1000.0f,
		VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
	mWoodTex = Texture2_CreateFromFile(Global::Context, "assets/textures/wood.png", true);
	mSandTex = Texture2_CreateFromFile(Global::Context, "assets/textures/sand.jpg", true);
	mStatueTex = Texture2_CreateFromFile(Global::Context, "assets/textures/statue.jpg", true);
	mNormalMap = Texture2_CreateFromFile(Global::Context, "assets/textures/normal.jpg", true);
	mLightDirection = glm::vec4(0.0);
	mQuery = vk::Gfx_CreateQueryPool(Global::Context, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);
	mInvocationQuery = vk::Gfx_CreateQueryPool(Global::Context, VK_QUERY_TYPE_PIPELINE_STATISTICS, gFrameOverlapCount, VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT);
	
	BindingDescription geometryPass[4];
	geometryPass[0].mBindingID = 0;
	geometryPass[0].mFlags = 0;
	geometryPass[0].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	geometryPass[0].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[0].mBufferSize = 0;
	geometryPass[0].mBuffer = verticesSSBO;
	geometryPass[0].mSharedResources = true;

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
	geometryPass[2].mSharedResources = true;

	geometryPass[3].mBindingID = 3;
	geometryPass[3].mFlags = 0;
	geometryPass[3].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	geometryPass[3].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[3].mBufferSize = 0;
	geometryPass[3].mBuffer = cullPass->mOutputDrawDataArray;
	geometryPass[3].mSharedResources = true;

	BindingDescription geometryPassFragment[3];
	geometryPassFragment[0].mBindingID = 0;
	geometryPassFragment[0].mFlags = 0;
	geometryPassFragment[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	geometryPassFragment[0].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPassFragment[0].mTextures.push_back(mWoodTex);
	geometryPassFragment[0].mSampler = Global::DefaultSampler;

	geometryPassFragment[1].mBindingID = 1;
	geometryPassFragment[1].mFlags = 0;
	geometryPassFragment[1].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	geometryPassFragment[1].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPassFragment[1].mTextures.push_back(shadowMap);
	geometryPassFragment[1].mSampler = mShadowSampler;

	geometryPassFragment[2].mBindingID = 2;
	geometryPassFragment[2].mFlags = BINDING_FLAG_CPU_VISIBLE | BINDING_FLAG_REQUIRE_COHERENT;
	geometryPassFragment[2].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	geometryPassFragment[2].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPassFragment[2].mBufferSize = sizeof(glm::vec3) + (lightCount * sizeof(ShaderTypes::Light));
	geometryPassFragment[2].mBuffer = nullptr;

	BindingDescription terrainBindings[6]{};
	terrainBindings[0].mBindingID = 0;
	terrainBindings[0].mFlags = 0;
	terrainBindings[0].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	terrainBindings[0].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	terrainBindings[0].mBufferSize = 0;
	terrainBindings[0].mBuffer = terrain->GetVerticesBuffer();
	terrainBindings[0].mSharedResources = true;

	terrainBindings[1].mBindingID = 1;
	terrainBindings[1].mFlags = 0;
	terrainBindings[1].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	terrainBindings[1].mStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainBindings[1].mBufferSize = 0;
	terrainBindings[1].mBuffer = nullptr;
	terrainBindings[1].mSharedResources = true;

	terrainBindings[2].mBindingID = 2;
	terrainBindings[2].mFlags = 0;
	terrainBindings[2].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	terrainBindings[2].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainBindings[2].mTextures.push_back(mSandTex);
	terrainBindings[2].mSampler = Global::DefaultSampler;

	terrainBindings[3].mBindingID = 3;
	terrainBindings[3].mFlags = 0;
	terrainBindings[3].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	terrainBindings[3].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainBindings[3].mTextures.push_back(shadowMap);
	terrainBindings[3].mSampler = mShadowSampler;

	terrainBindings[4].mBindingID = 4;
	terrainBindings[4].mFlags = 0;
	terrainBindings[4].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	terrainBindings[4].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainBindings[4].mTextures.push_back(mNormalMap);
	terrainBindings[4].mSampler = Global::DefaultSampler;

	terrainBindings[5].mBindingID = 5;
	terrainBindings[5].mFlags = 0;
	terrainBindings[5].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	terrainBindings[5].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorPoolSize> poolSizes;
	ShaderConnector_CalculateDescriptorPool(4, geometryPass, poolSizes);
	ShaderConnector_CalculateDescriptorPool(3, geometryPassFragment, poolSizes);
	ShaderConnector_CalculateDescriptorPool(6, terrainBindings, poolSizes);
	mPool = vk::Gfx_CreateDescriptorPool(Global::Context, gFrameOverlapCount * 2 + (gFrameOverlapCount * 1), poolSizes);
	
	mGeoSet0 = ShaderConnector_CreateSet(Global::Context, 0, mPool, 4, geometryPass);
	mGeoSet1 = ShaderConnector_CreateSet(Global::Context, 1, mPool, 3, geometryPassFragment);
	
	terrainBindings[5].mBuffer = mGeoSet1.GetBuffer2(2);
	terrainBindings[5].mSharedResources = true;

	DescriptorSet GeoSets[2] = { mGeoSet0, mGeoSet1 };
	mGeoLayout = ShaderConnector_CreatePipelineLayout(Global::Context, 2, GeoSets, {});
	Shader geoVertex = Shader(Global::Context, "assets/shaders/vertex.vert");
	Shader geoFragment = Shader(Global::Context, "assets/shaders/fragment.frag");
	geoFragment.SetSpecializationConstant<int>(0, lightCount);
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
	mGeoState = PipelineState_Create(Global::Context, spec, input, mFBO, mGeoLayout, &geoVertex, &geoFragment);

	terrainBindings[1].mBuffer = mGeoSet0.GetBuffer2(1);
	
	mMapSet = ShaderConnector_CreateSet(Global::Context, 0, mPool, 6, terrainBindings);
	mMapLayout = ShaderConnector_CreatePipelineLayout(Global::Context, 1, &mMapSet, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TerrainPushblock)} });
	Shader mapVertex = Shader(Global::Context, "assets/shaders/terrain/Terrain.vert");
	Shader mapFragment = Shader(Global::Context, "assets/shaders/terrain/Terrain.frag");
	mMapState = PipelineState_Create(Global::Context, mGeoState->m_spec, input, mFBO, mMapLayout, &mapVertex, &mapFragment);
	
	for (int i = 0; i < gFrameOverlapCount; i++) {
		RecordCommands(i);
	}
}

Application::GeometryPass::~GeometryPass() {
	Super_Pass_Destroy();
	Gfree(mTerrainDrawCommands);
	Gfree(mTerrainDrawCount);
	mFBO.DestroyAllBoundAttachments();
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	ShaderConnector_DestroySet(Global::Context->defaultDevice, mGeoSet0);
	ShaderConnector_DestroySet(Global::Context->defaultDevice, mGeoSet1);
	ShaderConnector_DestroySet(Global::Context->defaultDevice, mMapSet);
	vkDestroyPipelineLayout(mDevice, mGeoLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mMapLayout, nullptr);
	PipelineState_Destroy(mGeoState);
	PipelineState_Destroy(mMapState);
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
	glm::mat4 proj = Global::Projection;
	glm::mat4 view = mCamera->GetViewMatrix();

	auto globalDataBuffer = mGeoSet0.GetBuffer2(1);
	void* ptr = Buffer2_Map(globalDataBuffer);
	ShaderTypes::GlobalData data;
	data.u_LightDirection = mLightDirection;

	data.u_DeltaTime = dTime;
	data.u_TimeFromStart = dTimeFromStart;
	data.u_View = view;
	data.u_Projection = proj;
	data.u_ProjView = proj * view;
	data.u_LightSpace = mLightSpace;
	memcpy(ptr, &data, sizeof(data));
	Buffer2_Flush(globalDataBuffer, 0, VK_WHOLE_SIZE);
	CullTerrain(proj, mLockedCamera->GetViewMatrix());

	// update light info
	auto lightBuffer = mGeoSet1.GetBuffer2(2);
	auto cameraPos = mCamera->GetPosition();
	glm::vec3* lightPtr8 = (glm::vec3*)Buffer2_Map(lightBuffer);
	memcpy(lightPtr8, &cameraPos, sizeof(glm::vec3));
	ShaderTypes::Light* lightPtr = (ShaderTypes::Light*)(lightPtr8 + 1);
	for (uint32_t i = 0; i < mLightCount; i++) {
		memcpy(&lightPtr[i], &mLights[i], sizeof(ShaderTypes::Light));
	}
	Buffer2_Flush(lightBuffer, 0, VK_WHOLE_SIZE);

	return *mCmd;
}

void Application::GeometryPass::CullTerrain(const glm::mat4& proj, const glm::mat4& view) {
	using namespace glm;
	glm::mat4 transform = mT0->GetTransform();
	// Thanks to https://stackoverflow.com/questions/58211003/about-view-matrix-and-frustum-culling
	// and http://www.lighthouse3d.com/tutorials/view-frustum-culling/radar-approach-testing-points/
//#pragma message("@@@@@@@@@@Make Sure to remove this when done debugging@@@@@@@@@@@@@ In GeometryPass.cpp CullTerrain()")
	//mT0->SetTransform(glm::mat4(1.0));
	
	glm::mat4 projT = glm::transpose(proj * view);
	//glm::mat4 viewT = glm::transpose(view);
	glm::mat4 model = mT0->GetTransform();

	auto normalizePlane = [](glm::vec4 p) -> glm::vec4
	{
		return p / glm::length(glm::vec3(p));
	};
	glm::vec4 planes[5];
	planes[0] = normalizePlane(projT[3] + projT[0]);
	planes[1] = normalizePlane(projT[3] - projT[0]);
	planes[2] = normalizePlane(projT[3] + projT[1]);
	planes[3] = normalizePlane(projT[3] - projT[1]);
	planes[4] = normalizePlane(projT[3] - projT[2]);

	vec4 frustumX = normalizePlane(projT[3] + projT[0]); // x + w < 0
	vec4 frustumY = normalizePlane(projT[3] + projT[1]); // y + w < 0

	float frustrumF[4] = {
		frustumX.x,
		frustumX.z,
		frustumY.y,
		frustumY.z,
	};

	vec3 modelScale = vec3(model[0][0], model[1][1], model[2][2]);
	auto InsideFrustrum = [&](const Ph::BoundingSphere& sphere) throw() -> bool {
		return true;
		vec3 radiusxyz = modelScale * sphere.mRadius;
		// choose the biggest radius in any direction
		float radius = sphere.mRadius * 2.5f; // max(max(radiusxyz.x, radiusxyz.y), radiusxyz.z) * 1.0f;
		vec4 center = model * vec4(sphere.mCenter, 1.0);

		bool visible = true;
		//// the left/top/right/bottom plane culling utilizes frustum symmetry to cull against two planes at the same time
		//visible = visible && center.z * frustrumF[1] - abs(center.x) * frustrumF[0] < -radius;
		//visible = visible && center.z * frustrumF[3] - abs(center.y) * frustrumF[2] < radius;
#if 1
		for (int i = 0; i < 5; i++) {
		//	if (i == 3)
		//		continue;
			visible = visible && dot(planes[i], center) > -radius;
		}
#endif

		return visible;
	};

	uint32_t DrawCount = 0;
	for (uint32_t i = 0; i < mT0->GetSubmeshCount(); i++) {
		const auto& submesh = mT0->GetSubmesh(i);
		Ph::BoundingSphere sphere = submesh.mSphere;
		//sphere.mBoxMin = transform * glm::vec4(box.mBoxMin, 1.0f);
		//sphere.mBoxMax = transform * glm::vec4(box.mBoxMax, 1.0f);

		if (InsideFrustrum(sphere)) {
			VkDrawIndexedIndirectCommand command;
			command.indexCount = submesh.mIndicesCount;
			command.instanceCount = 1;
			command.firstIndex = submesh.mFirstIndex;
			command.vertexOffset = submesh.mFirstVertex;
			command.firstInstance = 0;
			memcpy(&mTerrainDrawCommands[DrawCount], &command, sizeof(command));
			DrawCount++;
		}
	}
	memcpy(mTerrainDrawCount, &DrawCount, 4);
	GverifyWrite(mTerrainDrawCommands);
}

ITexture2 Application::GeometryPass::CreateMinimap() {
	return GenerateMinimap(mT0, { mSandTex }, mNormalMap);
}

#if 0
void Application::GeometryPass::ChangeLights(const std::vector<Light>& lights)
{
}
#endif

void Application::GeometryPass::ReloadShaders() {
	PipelineSpecification specification = mGeoState->m_spec;
	PipelineState_Destroy(mGeoState);
	PipelineState_Destroy(mMapState);
	Shader geoVertex = Shader(Global::Context, "assets/shaders/vertex.vert");
	Shader geoFragment = Shader(Global::Context, "assets/shaders/fragment.frag");
	Shader mapVertex = Shader(Global::Context, "assets/shaders/terrain/Terrain.vert");
	Shader mapFragment = Shader(Global::Context, "assets/shaders/terrain/Terrain.frag");
	PipelineVertexInputDescription input;
	mGeoState = PipelineState_Create(Global::Context, specification, input, mFBO, mGeoLayout, &geoVertex, &geoFragment);
	mMapState = PipelineState_Create(Global::Context, specification, input, mFBO, mMapLayout, &mapVertex, &mapFragment);
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
	Shader geoVertex = Shader(Global::Context, "assets/shaders/vertex.vert");
	Shader geoFragment = Shader(Global::Context, "assets/shaders/fragment.frag");
	PipelineVertexInputDescription input;
	mGeoState = PipelineState_Create(Global::Context, specification, input, mFBO, mGeoLayout, &geoVertex, &geoFragment);
	PipelineState_Destroy(mMapState);
	Shader mapVertex = Shader(Global::Context, "assets/shaders/terrain/Terrain.vert");
	Shader mapFragment = Shader(Global::Context, "assets/shaders/terrain/Terrain.frag");
	mMapState = PipelineState_Create(Global::Context, specification, input, mFBO, mMapLayout, &mapVertex, &mapFragment);
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
		passTime = (double(data0[1] - data0[0]) * Global::Context->card.deviceLimits.timestampPeriod) * 1e-6;
		vertexInvocations = invocs[0];
		fragmentInvocations = invocs[1];
	}
}

void Application::GeometryPass::RecordCommands(uint32_t FrameIndex)
{
	uint32_t i = FrameIndex;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkCommandBuffer cmd = mCmd->mCmds[FrameIndex];
	mCmdPool->Reset(FrameIndex);
	vkBeginCommandBuffer(cmd, &beginInfo);
	vk::Gfx_InsertDebugLabel(cmd, FrameIndex, "Geometry Pass", 0.0, 1.0);

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

	VkRenderingAttachmentInfo colorAttachment = mFBO.GetRenderingAttachmentInfo(FrameIndex, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
	VkRenderingAttachmentInfo depthAttachment = mFBO.GetDepthRenderingAttachmentInfo(FrameIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthAttachment;
	renderingInfo.pStencilAttachment = nullptr;
	vkCmdBeginRenderingKHR(cmd, &renderingInfo);

	VkImageMemoryBarrier renderBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	renderBarrier.srcAccessMask = VK_ACCESS_NONE;
	renderBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	renderBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
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
	pushblock.u_Model = mT0->GetTransform();
	pushblock.u_NormalModel = glm::mat3(glm::transpose(glm::inverse(pushblock.u_Model)));

	VkDescriptorSet mapSet[1] = { mMapSet[FrameIndex] };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mMapState->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mMapLayout, 0, 1, mapSet, 0, nullptr);
	vkCmdPushConstants(cmd, mMapLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TerrainPushblock), &pushblock);
	vkCmdBindIndexBuffer(cmd, mT0->GetIndicesBuffer()->mBuffers[FrameIndex], 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirectCount(cmd, Gbuffer(mTerrainDrawCommands)->mBuffers[FrameIndex], 0, Gbuffer(mTerrainDrawCount)->mBuffers[FrameIndex], 0, mT0->GetSubmeshCount(), sizeof(VkDrawIndexedIndirectCommand));

	VkImageMemoryBarrier presentBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	presentBarrier.dstAccessMask = VK_ACCESS_NONE;
	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
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
	DvkCmdEndDebugUtilsLabelEXT(cmd);
	vkEndCommandBuffer(cmd);
}
