#include "ShadowPass.hpp"
#include <backend/VkGraphicsCard.hpp>
#include "../../window/PlatformWindow.hpp"
#include <shaders/Shader.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

extern vk::VkContext gContext;

namespace Application {
	extern PlatformWindow* gWindow;
}

std::vector<float> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view, const glm::mat4& lightView)
{
	const auto inv = glm::inverse(proj * view);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4 pt =
					inv * glm::vec4(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::min();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::min();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::min();
	for (const auto& v : frustumCorners)
	{
		const auto trf = lightView * v;
		minX = glm::min(minX, trf.x);
		maxX = glm::max(maxX, trf.x);
		minY = glm::min(minY, trf.y);
		maxY = glm::max(maxY, trf.y);
		minZ = glm::min(minZ, trf.z);
		maxZ = glm::max(maxZ, trf.z);
	}		   
	constexpr float zMult = 10.0f;
	if (minZ < 0)
	{
		minZ *= zMult;
	}
	else
	{
		minZ /= zMult;
	}
	if (maxZ < 0)
	{
		maxZ /= zMult;
	}
	else
	{
		maxZ *= zMult;
	}

	std::vector<float> frustrumCornersFloat;
	frustrumCornersFloat.push_back(minX);
	frustrumCornersFloat.push_back(maxX);
	frustrumCornersFloat.push_back(minY);
	frustrumCornersFloat.push_back(maxY);
	frustrumCornersFloat.push_back(minZ);
	frustrumCornersFloat.push_back(maxZ);

	return frustrumCornersFloat;
}


// TODO: Perform frustrm culling.
Application::ShadowPass::ShadowPass(IBuffer2 verticesSSBO, IBuffer2 indices, Terrain& terrain, EntityController* ecs, Camera* camera, int size) : Scene(gContext->defaultDevice, true), mVerticesSSBO(verticesSSBO), mIndices(indices), mECS(ecs), mSize(size), mCamera(camera) {
	mT0 = terrain;
	VkClearValue clear{};
	clear.depthStencil.depth = 0.0;
	mFBO.m_width = size;
	mFBO.m_height = size;
	FramebufferAttachment depthAtachment = FramebufferAttachment::Create(gContext, 0, size, size, VK_FORMAT_D32_SFLOAT, clear);
	mFBO.SetDepthAttachment(depthAtachment);
	auto cmd = vk::Gfx_CreateSingleUseCmdBuffer(gContext);
	Framebuffer_TransistionAttachment(cmd.cmd, &depthAtachment, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(cmd);

	Shader vertex = Shader(gContext, "assets/shaders/shadow/shadow.vert");
	Shader fragment = Shader(gContext, "assets/shaders/shadow/shadow.frag");

	BindingDescription bindings[4];
	bindings[0].mBindingID = 0;
	bindings[0].mFlags = 0;
	bindings[0].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].mBufferSize = 0;
	bindings[0].mBuffer = verticesSSBO;
	bindings[0].mInternalSharedResources = true;

	bindings[1].mBindingID = 1;
	bindings[1].mFlags = 0;
	bindings[1].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[1].mBufferSize = 0;
	bindings[1].mBuffer = ecs->GetGeometryDataBuffer();
	bindings[1].mInternalSharedResources = true;

	bindings[2].mBindingID = 2;
	bindings[2].mFlags = 0;
	bindings[2].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[2].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[2].mBufferSize = 0;
	bindings[2].mBuffer = ecs->GetDrawDataBuffer();
	bindings[2].mInternalSharedResources = true;

	bindings[3].mBindingID = 3;
	bindings[3].mFlags = BINDING_FLAG_CPU_VISIBLE;
	bindings[3].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[3].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[3].mBufferSize = sizeof(glm::mat4) * 2;
	bindings[3].mBuffer = nullptr;

	BindingDescription shadowTBindings[1];
	shadowTBindings[0].mBindingID = 0;
	shadowTBindings[0].mFlags = BINDING_FLAG_CPU_VISIBLE;
	shadowTBindings[0].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadowTBindings[0].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	shadowTBindings[0].mBufferSize = sizeof(glm::mat4);

	std::vector<VkDescriptorPoolSize> poolSize;
	ShaderConnector_CalculateDescriptorPool(4, bindings, poolSize);
	ShaderConnector_CalculateDescriptorPool(1, shadowTBindings, poolSize);
	mPool = vk::Gfx_CreateDescriptorPool(gContext, gFrameOverlapCount * 2, poolSize);
	mSet = ShaderConnector_CreateSet(0, mPool, 4, bindings, 0, nullptr);
	mLayout = ShaderConnector_CreatePipelineLayout(1, &mSet, {});

	PipelineVertexInputDescription input;
	PipelineSpecification spec;
	spec.m_CullMode = CullMode::CULL_FRONT;
	spec.m_DepthEnabled = true;
	spec.m_DepthWriteEnable = true;
	spec.m_DepthFunc = DepthFunction::GREATER;
	spec.m_PolygonMode = PolygonMode::FILL;
	spec.m_FrontFaceCCW = true;
	spec.m_Topology = PolygonTopology::TRIANGLE_LIST;
	spec.m_NearField = 0.0f;
	spec.m_FarField = 1.0f;
	mState = PipelineState_Create(gContext, spec, input, mFBO, mLayout, &vertex, &fragment);

	Shader shadowT = Shader(gContext, "assets/shaders/shadow/shadowT.vert");
	shadowTBindings[0].mBuffer = mSet.GetBuffer2(3);
	mTerrainSet = ShaderConnector_CreateSet(0, mPool, 1, shadowTBindings, 0, nullptr);
	mTerrainLayout = ShaderConnector_CreatePipelineLayout(1, &mTerrainSet, { });

	input = {};
	input.AddInputElement("inPosition", 0, 0, 3, true, false, false);
	input.AddInputElement("inNormal", 1, 0, 3, true, false, false);
	input.AddInputElement("inTangent", 2, 0, 3, true, false, false);
	input.AddInputElement("inBiTangent", 3, 0, 3, true, false, false);
	input.AddInputElement("inTextureIDs", 4, 0, 3, false, false, false);
	input.AddInputElement("inTextureWeights", 5, 0, 3, true, false, false);
	input.AddInputElement("inTexCoords", 6, 0, 2, true, false, false);
	spec.m_CullMode = CullMode::CULL_FRONT;
	mTerrainState = PipelineState_Create(gContext, spec, input, mFBO, mTerrainLayout, &shadowT, &fragment);

	mQuery = vk::Gfx_CreateQueryPool(gContext, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);

	for (int i = 0; i < gFrameOverlapCount; i++)
		RecordCommands(i);
}

Application::ShadowPass::~ShadowPass() {
	Super_Scene_Destroy();
	mFBO.DestroyAllBoundAttachments();
	PipelineState_Destroy(mState);
	PipelineState_Destroy(mTerrainState);
	ShaderConnector_DestroySet(mSet);
	vkDestroyQueryPool(mDevice, mQuery, nullptr);
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	vkDestroyPipelineLayout(mDevice, mLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mTerrainLayout, nullptr);
}

void Application::ShadowPass::ReloadShaders() {
	Shader vertex = Shader(gContext, "assets/shaders/shadow/shadow.vert");
	Shader fragment = Shader(gContext, "assets/shaders/shadow/shadow.frag");
	PipelineVertexInputDescription input;
	PipelineSpecification spec = mState->m_spec;
	PipelineState_Destroy(mState);
	mState = PipelineState_Create(gContext, spec, input, mFBO, mLayout, &vertex, &fragment);
	for (int i = 0; i < gFrameOverlapCount; i++)
		RecordCommands(i);
}

VkCommandBuffer Application::ShadowPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	glm::mat4 u_LightSpace = GetLightSpace();
	struct {
		glm::mat4 ShadowTModel;
		glm::mat4 ProjView;
	} MVPBinding;
	MVPBinding.ShadowTModel = mT0.mModelTransform;
	MVPBinding.ProjView = u_LightSpace;
	IBuffer2 uniform = mSet.GetBuffer2(3);
	void* mapped_ptr = Buffer2_Map(uniform);
	memcpy(mapped_ptr, &MVPBinding, sizeof(MVPBinding));
	Buffer2_Flush(uniform, 0, VK_WHOLE_SIZE);
	return mCmds[FrameIndex];
}

void Application::ShadowPass::GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime) {
	uint64_t data[2];
	VkResult q = vkGetQueryPoolResults(mDevice, mQuery, FrameIndex * 2, 2, sizeof(data), data, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (Wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	if (q == VK_SUCCESS) {
		dTime = (double(data[1] - data[0]) * gContext->card.deviceLimits.timestampPeriod) * 1e-6;
	}
}

glm::mat4 Application::ShadowPass::GetLightSpace()
{
	glm::mat4 proj = glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->GetWidth(), (float)gWindow->GetHeight(), 0.1f, 1000.0f);
	glm::mat4 view = mCamera->GetViewMatrix();
	glm::mat4 ip = glm::inverse(proj * view);
	glm::mat4 lightView = glm::lookAt(mLightPosition, glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));

	auto corners = getFrustumCornersWorldSpace(proj, view, lightView);

	glm::mat4 offset = glm::translate(glm::mat4(1.0), mCamera->GetPosition());
	float size = 125.0f;
	glm::mat4 lightSpace = glm::ortho(-size, size, -size, size, -size, size) * lightView;
	return lightSpace;
}

void Application::ShadowPass::RecordCommands(uint32_t FrameIndex)
{
	vkResetCommandPool(mDevice, mPools[FrameIndex], 0);
	VkCommandBuffer cmd = mCmds[FrameIndex];
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(cmd, &beginInfo);
	vkCmdResetQueryPool(cmd, mQuery, (FrameIndex * 2), 2);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQuery, (FrameIndex * 2));

	auto& depth = mFBO.m_depth_attachment.value();

	VkRenderingAttachmentInfoKHR attachmentInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	attachmentInfo.imageView = depth.GetView(FrameIndex);
	attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
	attachmentInfo.resolveImageView = nullptr;
	attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentInfo.clearValue = depth.mClear;

	VkRenderingInfoKHR renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
	renderingInfo.flags = 0;
	renderingInfo.renderArea = { {0, 0}, { mSize, mSize } };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 0;
	renderingInfo.pColorAttachments = nullptr;
	renderingInfo.pDepthAttachment = &attachmentInfo;
	renderingInfo.pStencilAttachment = nullptr;
	vkCmdBeginRenderingKHR(cmd, &renderingInfo);

	VkImageMemoryBarrier barrier0{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier0.srcAccessMask = VK_ACCESS_NONE;
	barrier0.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrier0.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier0.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier0.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier0.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier0.image = depth.GetImage(FrameIndex);
	barrier0.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barrier0.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier0.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier0);

	VkDescriptorSet set[1] = { mSet[FrameIndex] };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mState->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout, 0, 1, set, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, mIndices->mBuffers[FrameIndex], 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirect(cmd, mSet.GetBuffer(2, FrameIndex), 0, 1, sizeof(ShaderTypes::DrawData));

	VkBuffer vertices = mT0.mVertices->mBuffers[FrameIndex];
	VkBuffer indices = mT0.mIndices->mBuffers[FrameIndex];
	VkDeviceSize offset[1] = { 0 };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainState->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainLayout, 0, 1, &mTerrainSet[FrameIndex], 0, nullptr);
	vkCmdBindVertexBuffers(cmd, 0, 1, &vertices, offset);
	vkCmdBindIndexBuffer(cmd, indices, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmd, mT0.mIndicesCount, 1, 0, 0, 0);

	barrier0.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;
	barrier0.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier0.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier0.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier0);

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 1);
	vkCmdEndRenderingKHR(cmd);
	vkEndCommandBuffer(cmd);
}
