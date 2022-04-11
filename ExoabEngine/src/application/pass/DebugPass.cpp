#include "DebugPass.hpp"
#include <shaders/ShaderConnector.hpp>
#include "../Globals.hpp"

Application::DebugPass::DebugPass(const Framebuffer& fbo) : Pass(Global::Context->defaultDevice, true), mFBO(fbo), mProjView(glm::mat4(1.0)), mDebugObjectCount(0), mDebugBuffer(nullptr), mDebugObjectIndex(0), mDebugBufferPointer(0) {
	mLayout = ShaderConnector_CreatePipelineLayout(0, nullptr, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DebugPushblock)}});
	Shader vertexShader = Shader("assets/shaders/debug/debugVertex.vert");
	Shader fragmentShader = Shader("assets/shaders/debug/debugFragment.frag");
	PipelineSpecification specification;
	specification.m_CullMode = CullMode::CULL_BACK;
	specification.m_DepthEnabled = true;
	specification.m_DepthWriteEnable = true;
	specification.m_DepthFunc = DepthFunction::LESS;
	specification.m_PolygonMode = PolygonMode::FILL;
	specification.m_FrontFaceCCW = false;
	specification.m_Topology = PolygonTopology::TRIANGLE_LIST;
	specification.m_SampleRateShadingEnabled = false;
	specification.m_MinSampleShading = 0.0;
	specification.m_NearField = 0.0f;
	specification.m_FarField = 1.0f;
	PipelineVertexInputDescription input;
	mState = PipelineState_Create(Global::Context, specification, input, fbo, mLayout, &vertexShader, &fragmentShader);
	specification.m_DepthFunc = DepthFunction::ALWAYS;
	specification.m_DepthWriteEnable = false;
	specification.m_CullMode = CullMode::CULL_NONE;
	mStateInFront = PipelineState_Create(Global::Context, specification, input, fbo, mLayout, &vertexShader, &fragmentShader);
	mQuery = vk::Gfx_CreateQueryPool(Global::Context, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);
}

Application::DebugPass::~DebugPass() {
	Super_Pass_Destroy();
	vkDestroyPipelineLayout(mDevice, mLayout, nullptr);
	vkDestroyQueryPool(mDevice, mQuery, nullptr);
	PipelineState_Destroy(mState);
	PipelineState_Destroy(mStateInFront);
	if (mDebugBuffer)
		Gfree(mDebugBuffer);
}

void Application::DebugPass::ReloadShaders() {
	Shader vertexShader = Shader("assets/shaders/debug/debugVertex.vert");
	Shader fragmentShader = Shader("assets/shaders/debug/debugFragment.frag");
	PipelineSpecification specification = mState->m_spec;
	PipelineVertexInputDescription input;
	PipelineState_Destroy(mState);
	mState = PipelineState_Create(Global::Context, specification, input, mFBO, mLayout, &vertexShader, &fragmentShader, {});
	mStateInFront = PipelineState_Create(Global::Context, mStateInFront->m_spec, input, mFBO, mLayout, &vertexShader, &fragmentShader, {});
}

VkCommandBuffer Application::DebugPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	RecordCommands(FrameIndex);
	return *mCmd;
}

void Application::DebugPass::DrawCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color, bool inFront, uint32_t requiredSize) {
	BufferSizeCheck(requiredSize);
	DebugObject obj;
	obj.inOffset = position - (scale / 2.0f);
	obj.inScale = scale;
	obj.inColor = color;
	obj.inObjIndex = 0;
	// memcpy to avoid coherent memory read.
	memcpy(&mDebugBuffer[mDebugObjectIndex], &obj, sizeof(DebugObject));
	if (inFront)
		mCubesInFrontCount++;
	else
		mCubesCount++;
	mDebugObjectIndex++;
}

void Application::DebugPass::DrawLine(const glm::vec3& A, const glm::vec3& B, const glm::vec3& color, uint32_t requiredSize) {
	BufferSizeCheck(requiredSize);
	DebugObject obj;
	//obj.inOffset;
	//obj.inScale;
	obj.inPointAB[0] = A;
	obj.inPointAB[1] = B;
	obj.inColor = color;
	obj.inObjIndex = 1;
	memcpy(&mDebugBuffer[mDebugObjectIndex], &obj, sizeof(DebugObject));
	mLinesCount++;
	mDebugObjectIndex++;
}

void Application::DebugPass::BufferSizeCheck(uint32_t requiredSize) {
	if ((mDebugObjectCount == 0) || (mDebugObjectIndex + 1 == mDebugObjectCount) || (requiredSize > (mDebugObjectCount - mDebugObjectIndex))) {
		if (mDebugBuffer) {
			DebugObject* tempBuffer = new DebugObject[mDebugObjectCount];
			memcpy(tempBuffer, mDebugBuffer, sizeof(DebugObject) * mDebugObjectCount);
			Gfree(mDebugBuffer);
			uint32_t countPreIcremeant = mDebugObjectCount;
			mDebugObjectCount += (requiredSize) ? requiredSize : 10;
			mDebugBuffer = (DebugObject*)Gmalloc(mDebugObjectCount * sizeof(DebugObject), BufferType::BUFFER_TYPE_STORAGE, true);
			mDebugBufferPointer = Buffer2_GetGPUPointer(Gbuffer(mDebugBuffer));
			memcpy(mDebugBuffer, tempBuffer, sizeof(DebugObject) * countPreIcremeant);
			delete[] tempBuffer;
		}
		else {
			mDebugObjectCount += (requiredSize) ? requiredSize : 10;
			mDebugBuffer = (DebugObject*)Gmalloc(sizeof(DebugObject) * mDebugObjectCount, BufferType::BUFFER_TYPE_STORAGE, true);
			mDebugBufferPointer = Buffer2_GetGPUPointer(Gbuffer(mDebugBuffer));
		}
	}
}


void Application::DebugPass::GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime) {
	uint64_t data[2];
	VkResult q = vkGetQueryPoolResults(mDevice, mQuery, FrameIndex * 2, 2, sizeof(data), data, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (Wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	if (q == VK_SUCCESS) {
		dTime = (double(data[1] - data[0]) * Global::Context->card.deviceLimits.timestampPeriod) * 1e-6;
	}
}

void Application::DebugPass::RecordCommands(uint32_t FrameIndex) {
	VkCommandBuffer cmd = mCmd->mCmds[FrameIndex];
	mCmdPool->Reset(FrameIndex);

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(cmd, &beginInfo);
	vkCmdResetQueryPool(cmd, mQuery, (FrameIndex * 2), 2);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQuery, (FrameIndex * 2));

	VkRenderingInfo renderingInfo;
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = nullptr;
	renderingInfo.flags = VK_RENDERING_RESUMING_BIT_KHR | VK_RENDERING_SUSPENDING_BIT_KHR;
	renderingInfo.renderArea = { {0, 0}, { mFBO.m_width, mFBO.m_height } };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;

	VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	colorAttachment.imageView = mFBO.m_color_attachments[0].GetAttachment()->m_vk_views_per_frame[FrameIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	colorAttachment.resolveImageView = nullptr;
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = mFBO.m_color_attachments[0].mClear;
	VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthAttachment.imageView = mFBO.m_depth_attachment->GetAttachment()->m_vk_views_per_frame[FrameIndex];
	depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	depthAttachment.resolveImageView = nullptr;
	depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.clearValue.depthStencil.depth = 1.0f;

	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthAttachment;
	renderingInfo.pStencilAttachment = nullptr;
	vkCmdBeginRenderingKHR(cmd, &renderingInfo);

	if (mCubesCount > 0) {
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mState->m_pipeline);
		DebugPushblock pushblock;
		pushblock.u_ProjView = mProjView;
		pushblock.u_DebugObjectPtr = mDebugBufferPointer;
		pushblock.u_DebugInstanceOffset = 0;
		vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DebugPushblock), &pushblock);
		vkCmdDraw(cmd, 36, mCubesCount, 0, 0);
	}

	if (mCubesInFrontCount > 0 || mLinesCount > 0) {
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mStateInFront->m_pipeline);
		DebugPushblock pushblock;
		pushblock.u_ProjView = mProjView;
		pushblock.u_DebugObjectPtr = mDebugBufferPointer;
		pushblock.u_DebugInstanceOffset = mCubesCount;
		vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DebugPushblock), &pushblock);
		vkCmdDraw(cmd, 36, mCubesInFrontCount, 0, 0);
		
		pushblock.u_DebugInstanceOffset = mCubesCount + mCubesInFrontCount;
		vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DebugPushblock), &pushblock);
		vkCmdDraw(cmd, 6, mLinesCount, 0, 0);
	}

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 1);
	vkCmdEndRenderingKHR(cmd);

	vkEndCommandBuffer(cmd);
}
