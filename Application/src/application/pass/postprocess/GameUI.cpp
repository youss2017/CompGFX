#include "GameUI.hpp"
#include "Globals.hpp"
#include <stb/stb_image.h>
#include <cstddef> // offfsetof macro

struct UIVertex {
	glm::vec2 inPosition;
	glm::vec2 inTexCoord;
};

struct UIVerticesList {
	UIVertex QUAD0[6];
	UIVertex QUAD1[6];
	UIVertex QUAD2[6];
	UIVertex LINEQUAD[8];
};

Application::GameUI::GameUI(Framebuffer& targetFBO, int colorAttachmentIndex, ITexture2 minimap) : Pass(Global::Context->defaultDevice, true) {
	Shader vertex = Shader("assets/shaders/postprocess/ui/ui.vert");
	Shader fragment = Shader("assets/shaders/postprocess/ui/ui.frag");

	mCursor = Texture2_CreateFromFile("assets/textures/cursor.png", false);

	BindingDescription bindings[1]{};
	bindings[0].mBindingID = 0;
	bindings[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].mTextures.push_back(mCursor);
	bindings[0].mTextures.push_back(minimap);
	bindings[0].mSampler = Global::DefaultSampler;

	std::vector<VkDescriptorPoolSize> poolSizes;
	ShaderConnector_CalculateDescriptorPool(1, bindings, poolSizes);
	mPool = vk::Gfx_CreateDescriptorPool(gFrameOverlapCount, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
	mTextureSets.push_back(ShaderConnector_CreateSet(0, mPool, 1, bindings));

	mLayout = ShaderConnector_CreatePipelineLayout(1, &mTextureSets[0], { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec3)}, {VK_SHADER_STAGE_FRAGMENT_BIT, 16, 8} });
	PipelineSpecification spec;
	spec.m_CullMode = CullMode::CULL_NONE;
	spec.m_DepthEnabled = false;
	spec.m_DepthWriteEnable = false;
	spec.m_DepthFunc = DepthFunction::ALWAYS;
	spec.m_PolygonMode = PolygonMode::FILL;
	spec.m_FrontFaceCCW = true;
	spec.m_Topology = PolygonTopology::TRIANGLE_LIST;
	spec.m_SampleRateShadingEnabled = false;
	spec.m_MinSampleShading = 0.0f;
	spec.m_NearField = 0.0f;
	spec.m_FarField = 1.0f;
	PipelineVertexInputDescription input;
	input.AddInputElement("inPosition", 0, 0, 2, true, false, false);
	input.AddInputElement("inTexCoord", 1, 0, 2, true, false, false);

	mFBO.m_width = targetFBO.m_width;
	mFBO.m_height = targetFBO.m_height;
	mFBO.AddColorAttachment(0, targetFBO.m_color_attachments[colorAttachmentIndex]);

	auto& blendState = mFBO.m_color_attachments[0].m_blend_state;
	blendState = {};
	blendState.blendEnable = VK_TRUE;
	blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.colorBlendOp = VK_BLEND_OP_ADD;
	blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	mState = PipelineState_Create(spec, input, mFBO, mLayout, &vertex, &fragment);

	spec.m_Topology = PolygonTopology::LINE_LIST;
	spec.m_LineWidth = 5.0f;
	mLineListState = PipelineState_Create(spec, input, mFBO, mLayout, &vertex, &fragment, {VK_DYNAMIC_STATE_LINE_WIDTH});
	mVertices = (UIVerticesList*)Gmalloc(sizeof(UIVerticesList), BUFFER_TYPE_VERTEX, true);
}

Application::GameUI::~GameUI() {
	Super_Pass_Destroy();
	Gfree(mVertices);
	vkDestroyPipelineLayout(Global::Context->defaultDevice, mLayout, nullptr);
	Texture2_Destroy(mCursor);
	for (auto& set : mTextureSets)
		ShaderConnector_DestroySet(set);
	vkDestroyDescriptorPool(Global::Context->defaultDevice, mPool, nullptr);
	PipelineState_Destroy(mState);
	PipelineState_Destroy(mLineListState);
}

void Application::GameUI::SetCursorPosition(const Ph::Ray& ray) {
	using namespace glm;
	vec2 windowSize = vec2(Global::Window->GetWidth(), Global::Window->GetHeight());
	vec2 pos = vec2(ray.mOrigin) / windowSize;
	vec2 direction = normalize((vec2(ray.mDirection) / windowSize) - pos);
	pos = pos * 2.0f - 1.0f;

	direction *= vec2(1.0f, -1.0f);

	// Why doesn't glm support these?
	mat2 rotation = mat2(1.0);
	rotation[0][0] = direction.y;
	rotation[0][1] = direction.x;

	rotation[1][0] = -direction.x;
	rotation[1][1] = direction.y;

	// reduce size
	mat2 reduceSize = glm::mat4(0.50);
	mat2 transform = rotation * reduceSize;

	vec2 toCenter = vec2(0.0) - pos;

	vec2 cursorSize = vec2(mCursor->m_specification.m_Width, mCursor->m_specification.m_Height) / windowSize;
	vec2 cursorSizeHalf = cursorSize / 2.0f;
	UIVerticesList* vertices = (UIVerticesList*)mVertices;
	vertices->QUAD1[0].inPosition = (transform * (vec2(pos) + vec2(-cursorSizeHalf.x, -cursorSizeHalf.y) + toCenter)) - toCenter;
	vertices->QUAD1[0].inTexCoord = vec2(0.0, 0.0);
	vertices->QUAD1[1].inPosition = (transform * (vec2(pos) + vec2(cursorSizeHalf.x, -cursorSizeHalf.y) + toCenter)) - toCenter;
	vertices->QUAD1[1].inTexCoord = vec2(1.0, 0.0);
	vertices->QUAD1[2].inPosition = (transform * (vec2(pos) + vec2(cursorSizeHalf.x, cursorSizeHalf.y) + toCenter)) - toCenter;
	vertices->QUAD1[2].inTexCoord = vec2(1.0, 1.0);

	vertices->QUAD1[3].inPosition = (transform * (vec2(pos) + vec2(-cursorSizeHalf.x, -cursorSizeHalf.y) + toCenter)) - toCenter;
	vertices->QUAD1[3].inTexCoord = vec2(0.0, 0.0);
	vertices->QUAD1[4].inPosition = (transform * (vec2(pos) + vec2(cursorSizeHalf.x, cursorSizeHalf.y) + toCenter)) - toCenter;
	vertices->QUAD1[4].inTexCoord = vec2(1.0, 1.0);
	vertices->QUAD1[5].inPosition = (transform * (vec2(pos) + vec2(-cursorSizeHalf.x, cursorSizeHalf.y) + toCenter)) - toCenter;
	vertices->QUAD1[5].inTexCoord = vec2(0.0, 1.0);
}

void Application::GameUI::ReloadShaders() {
	Shader vertex = Shader("assets/shaders/postprocess/ui/ui.vert");
	Shader fragment = Shader("assets/shaders/postprocess/ui/ui.frag");
	PipelineSpecification spec = mState->m_spec;
	PipelineVertexInputDescription input;
	input.AddInputElement("inPosition", 0, 0, 2, true, false, false);
	input.AddInputElement("inTexCoord", 1, 0, 2, true, false, false);
	PipelineState_Destroy(mState);
	mState = PipelineState_Create(spec, input, mFBO, mLayout, &vertex, &fragment);
	spec = mLineListState->m_spec;
	mLineListState = PipelineState_Create(spec, input, mFBO, mLayout, &vertex, &fragment);
}

VkCommandBuffer Application::GameUI::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	using namespace glm;
	UIVerticesList* vertices = (UIVerticesList*)mVertices;
	vertices->QUAD0[0] = { vec2(-1.0, 0.6) };
	vertices->QUAD0[1] = { vec2(1.0, 1.0) };
	vertices->QUAD0[2] = { vec2(-1.0, 1.0) };

	vertices->QUAD0[3] = { vec2(-1.0, 0.6) };
	vertices->QUAD0[4] = { vec2(1.0, 0.6) };
	vertices->QUAD0[5] = { vec2(1.0, 1.0) };

	vertices->QUAD2[0] = { vec2(-1.0, 0.45), vec2(0.0, 0.0) };
	vertices->QUAD2[1] = { vec2(-0.55, 1.0), vec2(1.0, 1.0) };
	vertices->QUAD2[2] = { vec2(-1.0, 1.0), vec2(0.0, 1.0) };

	vertices->QUAD2[3] = { vec2(-1.0, 0.45), vec2(0.0, 0.0) };
	vertices->QUAD2[4] = { vec2(-0.55, 0.45), vec2(1.0, 0.0) };
	vertices->QUAD2[5] = { vec2(-0.55, 1.0), vec2(1.0, 1.0) };

	if (sMouseSelect.bEnable) {
		const vec2& A = sMouseSelect.A;
		const vec2& B = sMouseSelect.B;
		// --
		vertices->LINEQUAD[0].inPosition = vec2(A.x, A.y);
		vertices->LINEQUAD[1].inPosition = vec2(B.x, A.y);
		// ..|
		vertices->LINEQUAD[2].inPosition = vec2(B.x, A.y);
		vertices->LINEQUAD[3].inPosition = vec2(B.x, B.y);
		// __
		vertices->LINEQUAD[4].inPosition = vec2(A.x, B.y);
		vertices->LINEQUAD[5].inPosition = vec2(B.x, B.y);
		// |..
		vertices->LINEQUAD[6].inPosition = vec2(A.x, A.y);
		vertices->LINEQUAD[7].inPosition = vec2(A.x, B.y);
	}

	RecordCommands(FrameIndex);
	return *mCmd;
}

void Application::GameUI::SetMinimap(ITexture2 minimap) {
	BindingDescription bindings[1]{};
	bindings[0].mBindingID = 0;
	bindings[0].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].mTextures.push_back(mCursor);
	bindings[0].mTextures.push_back(minimap);
	bindings[0].mSampler = Global::DefaultSampler;
	vkFreeDescriptorSets(Global::Context->defaultDevice, mPool, 3, &mTextureSets[0].mSets[0]);
	mTextureSets[0] = ShaderConnector_CreateSet(0, mPool, 1, bindings);
}

void Application::GameUI::RecordCommands(uint32_t FrameIndex) {
	mCmdPool->Reset();
	VkCommandBuffer cmd = *mCmd;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(cmd, &beginInfo);
	vk::Gfx_InsertDebugLabel(Global::Context->defaultDevice, cmd, FrameIndex, "[PostProcess] Game UI", 0.0, 0.0, 1.0);


	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.flags = VK_RENDERING_RESUMING_BIT_KHR;
	renderingInfo.renderArea.extent = { mFBO.m_width, mFBO.m_height };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;
	auto color = mFBO.m_color_attachments[0];
	VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	colorAttachment.imageView = color.GetView(FrameIndex);
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
	colorAttachment.resolveImageView = nullptr;
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = color.mClear;

	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = nullptr;
	renderingInfo.pStencilAttachment = nullptr;

	vkCmdBeginRendering(cmd, &renderingInfo);

	VkDeviceSize pOffset[1] = { 0 };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mState->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout, 0, 1, &mTextureSets[0][FrameIndex], 0, nullptr);
	vkCmdBindVertexBuffers(cmd, 0, 1, &Gbuffer(mVertices)->mBuffers[FrameIndex], pOffset);
	//vkCmdSetPrimitiveTopology(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	glm::vec3 col = glm::vec3(0.1);
	struct {
		int u_UseTexture;
		int u_TextureID;
	} pushblock;
	pushblock.u_UseTexture = 0;
	pushblock.u_TextureID = 0;
	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(col), &col);

	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 16, sizeof(pushblock), &pushblock);
	vkCmdDraw(cmd, 6, 1, offsetof(UIVerticesList, QUAD0), 0);

	pushblock.u_UseTexture = 1;
	pushblock.u_TextureID = 0;
	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 16, sizeof(pushblock), &pushblock);
	vkCmdDraw(cmd, 6, 1, offsetof(UIVerticesList, QUAD1), 0);

	pushblock.u_UseTexture = 0;
	pushblock.u_TextureID = 1;
	col = glm::vec3(0.0);
	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(col), &col);
	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 16, sizeof(pushblock), &pushblock);
	vkCmdDraw(cmd, 6, 1, offsetof(UIVerticesList, QUAD2), 0);

	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 16, sizeof(pushblock), &pushblock);
	vkCmdDraw(cmd, 6, 1, offsetof(UIVerticesList, QUAD2), 0);

	if (sMouseSelect.bEnable) {
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLineListState->m_pipeline);
		pushblock.u_UseTexture = 0;
		col = glm::vec3(14, 250, 36) / glm::vec3(255.0f);
		vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(col), &col);
		vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 16, sizeof(pushblock), &pushblock);
		vkCmdSetLineWidth(cmd, 5.0f);
		//vkCmdSetPrimitiveTopology(cmd, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		vkCmdDraw(cmd, 8, 1, (3 * 6 * sizeof(UIVertex)), 0);
	}

	vkCmdEndRendering(cmd);
	DvkCmdEndDebugUtilsLabelEXT(cmd);
	vkEndCommandBuffer(cmd);
}
