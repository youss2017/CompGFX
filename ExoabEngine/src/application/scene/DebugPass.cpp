#include "DebugPass.hpp"
#include <shaders/ShaderConnector.hpp>

extern vk::VkContext gContext;

Application::DebugPass::DebugPass(const Framebuffer& fbo) : Scene(gContext->defaultDevice, true), mFBO(fbo), mProjView(glm::mat4(1.0)) {
	mLayout = ShaderConnector_CreatePipelineLayout(0, nullptr, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DebugObject)}});
	Shader vertexShader = Shader(gContext, "assets/shaders/debug/debugVertex.vert");
	Shader fragmentShader = Shader(gContext, "assets/shaders/debug/debugFragment.frag");
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
	mState = PipelineState_Create(gContext, specification, input, fbo, mLayout, &vertexShader, &fragmentShader, {} );
	mQuery = vk::Gfx_CreateQueryPool(gContext, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);
}

Application::DebugPass::~DebugPass() {
	Super_Scene_Destroy();
	vkDestroyPipelineLayout(mDevice, mLayout, nullptr);
	vkDestroyQueryPool(mDevice, mQuery, nullptr);
	PipelineState_Destroy(mState);
}

void Application::DebugPass::ReloadShaders() {
	Shader vertexShader = Shader(gContext, "assets/shaders/debug/debugVertex.vert");
	Shader fragmentShader = Shader(gContext, "assets/shaders/debug/debugFragment.frag");
	PipelineSpecification specification = mState->m_spec;
	PipelineVertexInputDescription input;
	PipelineState_Destroy(mState);
	mState = PipelineState_Create(gContext, specification, input, mFBO, mLayout, &vertexShader, &fragmentShader, {});

}

VkCommandBuffer Application::DebugPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	RecordCommands(FrameIndex);
	return mCmds[FrameIndex];
}

void Application::DebugPass::DrawCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color) {
	DebugObject obj;
	obj.inOffset = position;
	obj.inScale = scale;
	obj.inColor = color;
	obj.u_ProjView = mProjView;
	mCubes.push_back(obj);
}

void Application::DebugPass::GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime) {
	uint64_t data[2];
	VkResult q = vkGetQueryPoolResults(mDevice, mQuery, FrameIndex * 2, 2, sizeof(data), data, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (Wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	if (q == VK_SUCCESS) {
		dTime = (double(data[1] - data[0]) * gContext->card.deviceLimits.timestampPeriod) * 1e-6;
	}
}

void Application::DebugPass::RecordCommands(uint32_t FrameIndex) {
	VkCommandBuffer cmd = mCmds[FrameIndex];
	vkResetCommandPool(mDevice, mPools[FrameIndex], 0);

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(cmd, &beginInfo);
	vkCmdResetQueryPool(cmd, mQuery, (FrameIndex * 2), 2);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQuery, (FrameIndex * 2));

	VkRenderingInfo renderingInfo;
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = nullptr;
	renderingInfo.flags = VK_RENDERING_RESUMING_BIT;
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

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mState->m_pipeline);

	for (const auto& cube : mCubes) {
		vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DebugObject), &cube);
		vkCmdDraw(cmd, 36, 1, 0, 0);
	}

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQuery, (FrameIndex * 2) + 1);
	vkCmdEndRenderingKHR(cmd);
	vkCmdEndRenderingKHR(cmd);

	vkEndCommandBuffer(cmd);
}
