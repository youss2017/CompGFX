#include "ShadowPass.hpp"
#include <backend/VkGraphicsCard.hpp>
#include "../../window/PlatformWindow.hpp"
#include <shaders/Shader.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern vk::VkContext gContext;

constexpr float OFFSET = 10;
constexpr glm::vec4 UP = glm::vec4(0, 1, 0, 0);
constexpr glm::vec4 FORWARD = glm::vec4(0, 0, -1, 0);
constexpr float SHADOW_DISTANCE = 100;

namespace Application {
	extern PlatformWindow* gWindow;
}


// TODO: Perform frustrm culling.
Application::ShadowPass::ShadowPass(IBuffer2 verticesSSBO, IBuffer2 indices, EntityController* ecs, Camera* camera, int size) : Scene(gContext->defaultDevice, true), mVerticesSSBO(verticesSSBO), mIndices(indices), mECS(ecs), mSize(size), mCamera(camera) {
	VkClearValue clear{};
	clear.depthStencil.depth = 1.0;
	mFBO.m_width = size;
	mFBO.m_height = size;
	FramebufferAttachment depthAtachment = FramebufferAttachment::Create(gContext, size, size, VK_FORMAT_D32_SFLOAT, clear);
	mFBO.SetDepthAttachment(depthAtachment);
	auto cmd = vk::Gfx_CreateSingleUseCmdBuffer(gContext);
	Framebuffer_TransistionAttachment(cmd.cmd, &depthAtachment, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(cmd);

	Shader vertex = Shader(gContext, "assets/shaders/shadow/shadow.vert");
	Shader fragment = Shader(gContext, "assets/shaders/shadow/shadow.frag");

	std::vector<ShaderBinding> bindings(4);
	bindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[0].m_bindingID = 0;
	bindings[0].m_hostvisible = false;
	bindings[0].m_useclientbuffer = true;
	bindings[0].m_preinitalized = false;
	bindings[0].m_additional_buffer_flags = (BufferType)0;
	bindings[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].m_client_buffer = verticesSSBO;

	bindings[1].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[1].m_bindingID = 1;
	bindings[1].m_hostvisible = false;
	bindings[1].m_useclientbuffer = false;
	bindings[1].m_preinitalized = true;
	bindings[1].m_additional_buffer_flags = (BufferType)0;
	bindings[1].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[1].m_ssbo = ecs->GetGeometryDataArray();

	bindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[2].m_bindingID = 2;
	bindings[2].m_hostvisible = false;
	bindings[2].m_useclientbuffer = false;
	bindings[2].m_preinitalized = true;
	bindings[2].m_additional_buffer_flags = (BufferType)0;
	bindings[2].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[2].m_ssbo = ecs->GetDrawDataArray();

	bindings[3].m_type = SHADER_BINDING_UNIFORM_BUFFER;
	bindings[3].m_bindingID = 3;
	bindings[3].m_hostvisible = true;
	bindings[3].m_useclientbuffer = false;
	bindings[3].m_preinitalized = false;
	bindings[3].m_additional_buffer_flags = (BufferType)0;
	bindings[3].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[3].m_size = sizeof(glm::mat4);

	std::vector<VkDescriptorPoolSize> poolSize;
	ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSize, &bindings);
	mPool = vk::Gfx_CreateDescriptorPool(gContext, gFrameOverlapCount, poolSize);
	mSet = ShaderBinding_Create(gContext, mPool, 0, &bindings);
	mLayout = ShaderBinding_CreatePipelineLayout(gContext, { mSet }, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)} });

	PipelineVertexInputDescription input;
	PipelineSpecification spec;
	spec.m_CullMode = CullMode::CULL_FRONT;
	spec.m_DepthEnabled = true;
	spec.m_DepthWriteEnable = true;
	spec.m_DepthFunc = DepthFunction::LESS;
	spec.m_PolygonMode = PolygonMode::FILL;
	spec.m_FrontFaceCCW = true;
	spec.m_Topology = PolygonTopology::TRIANGLE_LIST;
	spec.m_NearField = 0.0f;
	spec.m_FarField = 1.0f;
	mState = PipelineState_Create(gContext, spec, input, mFBO, mLayout, &vertex, &fragment);

	for (int i = 0; i < gFrameOverlapCount; i++)
		RecordCommands(i);
}

Application::ShadowPass::~ShadowPass()
{
	Super_Scene_Destroy();
	mFBO.DestroyAllBoundAttachments();
	PipelineState_Destroy(mState);
	ShaderBinding_DestroySets(gContext, { mSet });
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	vkDestroyPipelineLayout(mDevice, mLayout, nullptr);
}

void Application::ShadowPass::ReloadShaders()
{
	Shader vertex = Shader(gContext, "assets/shaders/shadow/shadow.vert");
	Shader fragment = Shader(gContext, "assets/shaders/shadow/shadow.frag");
	PipelineVertexInputDescription input;
	PipelineSpecification spec = mState->m_spec;
	PipelineState_Destroy(mState);
	mState = PipelineState_Create(gContext, spec, input, mFBO, mLayout, &vertex, &fragment);
	for (int i = 0; i < gFrameOverlapCount; i++)
		RecordCommands(i);
}

VkCommandBuffer Application::ShadowPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart)
{
	glm::mat4 u_LightSpace = GetLightSpace();
	IBuffer2 uniform = mSet->GetBuffer2(3, FrameIndex);
	void* mapped_ptr = Buffer2_Map(uniform);
	memcpy(mapped_ptr, &u_LightSpace, sizeof(glm::mat4));
	Buffer2_Flush(uniform, 0, VK_WHOLE_SIZE);
	return mCmds[FrameIndex];
}

glm::mat4 Application::ShadowPass::GetLightSpace()
{
	glm::vec3 pos = mLightPosition;
	float size = 500.0;
	// 
	//glm::perspective(90.0f, 1.0f, 0.1f, 1000.0f)
	glm::mat4 lightSpace = glm::ortho(-100.0f, +100.0f, -100.0f, +100.0f, -100.0f, +100.0f) * glm::lookAt(pos, glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));
	return lightSpace;
}

void Application::ShadowPass::RecordCommands(uint32_t FrameIndex)
{
	vkResetCommandPool(mDevice, mPools[FrameIndex], 0);
	VkCommandBuffer cmd = mCmds[FrameIndex];
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(cmd, &beginInfo);

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

	VkDescriptorSet set[1] = { mSet->m_set[FrameIndex] };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mState->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout, 0, 1, set, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, mIndices->mBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirect(cmd, mSet->GetBuffer(2, FrameIndex), 0, 1, sizeof(ShaderTypes::DrawData));

	barrier0.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;
	barrier0.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier0.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier0.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier0);

	vkCmdEndRenderingKHR(cmd);
	vkEndCommandBuffer(cmd);
}
