#include "SkyboxPass.hpp"
#include "shaders/Shader.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../window/PlatformWindow.hpp"

extern vk::VkContext gContext;
namespace Application {
	extern PlatformWindow* gWindow;
}


Application::SkyboxPass::SkyboxPass(const std::string& environmentMapPath, GeometryPass* geoPass, Camera* camera, bool UsingDebugPass) : Scene(gContext->defaultDevice, true), mCubeMap(CubeMap_Create(environmentMapPath, VK_FORMAT_R8G8B8A8_UNORM)), mCamera(camera), mGeoPass(geoPass), mUsingDebugPass(UsingDebugPass) {
	mSampler = vk::Gfx_CreateSampler(gContext, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	std::vector<ShaderBinding> bindings(1);
	bindings[0].m_type = SHADER_BINDING_COMBINED_TEXTURE_SAMPLER;
	bindings[0].m_bindingID = 0;
	bindings[0].m_shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].m_sampler.push_back(mSampler);
	bindings[0].m_textures.push_back(mCubeMap);
	bindings[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	std::vector<VkDescriptorPoolSize> poolSizes;
	ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSizes, &bindings);
	mPool = vk::Gfx_CreateDescriptorPool(gContext, gFrameOverlapCount, poolSizes);
	mSet = ShaderBinding_Create(gContext, mPool, 0, &bindings);
	mLayout = ShaderBinding_CreatePipelineLayout(gContext, { mSet }, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)}, {VK_SHADER_STAGE_FRAGMENT_BIT, 64, 4} });

	Shader skyboxVert = Shader(gContext, "assets/shaders/skybox.vert");
	Shader skyboxFrag = Shader(gContext, "assets/shaders/skybox.frag");

	PipelineSpecification spec;
	spec.m_CullMode = CullMode::CULL_BACK;
	spec.m_DepthEnabled = true;
	spec.m_DepthWriteEnable = false;
	spec.m_DepthFunc = DepthFunction::LESS;
	spec.m_PolygonMode = PolygonMode::FILL;
	spec.m_FrontFaceCCW = true;
	spec.m_Topology = PolygonTopology::TRIANGLE_LIST;
	spec.m_SampleRateShadingEnabled = false;
	PipelineVertexInputDescription input;
	mState = PipelineState_Create(gContext, spec, input, mGeoPass->mFBO, mLayout, &skyboxVert, &skyboxFrag);

}

Application::SkyboxPass::~SkyboxPass()
{
	Super_Scene_Destroy();
	Texture2_Destroy(mCubeMap);
	vkDestroySampler(mDevice, mSampler, nullptr);
	vkDestroyPipelineLayout(mDevice, mLayout, nullptr);
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	ShaderBinding_DestroySets(gContext, { mSet });
	PipelineState_Destroy(mState);
}

void Application::SkyboxPass::ReloadShaders() {
	Shader skyboxVert = Shader(gContext, "assets/shaders/skybox.vert");
	Shader skyboxFrag = Shader(gContext, "assets/shaders/skybox.frag");
	PipelineSpecification spec = mState->m_spec;
	PipelineVertexInputDescription input;
	PipelineState_Destroy(mState);
	mState = PipelineState_Create(gContext, spec, input, mGeoPass->mFBO, mLayout, &skyboxVert, &skyboxFrag);
}

VkCommandBuffer Application::SkyboxPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	RecordCommands(FrameIndex);
	return mCmds[FrameIndex];
}

void Application::SkyboxPass::RecordCommands(uint32_t FrameIndex)
{
	// We have to reset command pool to update push constants
	vkResetCommandPool(mDevice, mPools[FrameIndex], 0);
	VkFormat colorFormat = mGeoPass->mFBO.m_color_attachments[0].GetFormat();
	VkFormat depthFormat = mGeoPass->mFBO.m_depth_attachment.value().GetFormat();

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkCommandBuffer cmd = mCmds[FrameIndex];
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderingInfo renderingInfo;
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = nullptr;
	renderingInfo.flags = mUsingDebugPass ? (VK_RENDERING_RESUMING_BIT | VK_RENDERING_SUSPENDING_BIT) : VK_RENDERING_RESUMING_BIT;
	renderingInfo.renderArea = { {0, 0}, { mGeoPass->mFBO.m_width, mGeoPass->mFBO.m_height } };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;

	VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	colorAttachment.imageView = mGeoPass->mFBO.m_color_attachments[0].GetAttachment()->m_vk_views_per_frame[FrameIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	colorAttachment.resolveImageView = nullptr;
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = mGeoPass->mFBO.m_color_attachments[0].mClear;
	VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthAttachment.imageView = mGeoPass->mFBO.m_depth_attachment->GetAttachment()->m_vk_views_per_frame[FrameIndex];
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
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout, 0, 1, &mSet->m_set[FrameIndex], 0, nullptr);
	glm::mat4 view = mCamera->GetViewMatrix();
	glm::mat4 proj = glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->GetWidth(), (float)gWindow->GetHeight(), 0.1f, 1000.0f);
	glm::mat4 u_ProjView = proj * glm::mat4(glm::mat3(view));
	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &u_ProjView);
	vkCmdPushConstants(cmd, mLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, 4, &mLod);
	vkCmdDraw(cmd, 36, 1, 0, 0);

	vkCmdEndRenderingKHR(cmd);

	vkEndCommandBuffer(cmd);
}
