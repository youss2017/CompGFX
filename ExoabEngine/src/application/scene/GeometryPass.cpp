#include "GeometryPass.hpp"
#include <shaders/ShaderBinding.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../window/PlatformWindow.hpp"

extern vk::VkContext gContext;
namespace Application {
	extern PlatformWindow* gWindow;
}

Application::GeometryPass::GeometryPass(IBuffer2 verticesSSBO, IBuffer2 indicesSSBO, MaterialConfiguration& geoConfig, Framebuffer& fbo, FrustumCullPass* cullPass, Camera* camera, EntityController* ecs) : Scene(gContext->defaultDevice), mCamera(camera), mECS(ecs) {
	
	mSampler = vk::Gfx_CreateSampler(gContext);
	mWoodTex = Texture2_CreateFromFile(gContext, "assets/textures/wood.png", true);
	mStatueTex = Texture2_CreateFromFile(gContext, "assets/textures/statue.jpg", true);
	
	std::vector<ShaderBinding> geometryPass(4);
	geometryPass[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	geometryPass[0].m_bindingID = 0;
	geometryPass[0].m_hostvisible = false;
	geometryPass[0].m_useclientbuffer = true;
	geometryPass[0].m_preinitalized = false;
	geometryPass[0].m_additional_buffer_flags = (BufferType)0;
	geometryPass[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[0].m_size = 0;
	geometryPass[0].m_client_buffer = verticesSSBO;

	geometryPass[1].m_type = SHADER_BINDING_UNIFORM_BUFFER;
	geometryPass[1].m_bindingID = 1;
	geometryPass[1].m_hostvisible = true;
	geometryPass[1].m_useclientbuffer = false;
	geometryPass[1].m_preinitalized = false;
	geometryPass[1].m_additional_buffer_flags = (BufferType)0;
	geometryPass[1].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[1].m_size = sizeof(ShaderTypes::GlobalData);

	geometryPass[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	geometryPass[2].m_bindingID = 2;
	geometryPass[2].m_hostvisible = false;
	geometryPass[2].m_useclientbuffer = false;
	geometryPass[2].m_preinitalized = true;
	geometryPass[2].m_additional_buffer_flags = (BufferType)0;
	geometryPass[2].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[2].m_ssbo = cullPass->mOutputGeometryDataArray;

	geometryPass[3].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	geometryPass[3].m_bindingID = 3;
	geometryPass[3].m_hostvisible = false;
	geometryPass[3].m_useclientbuffer = false;
	geometryPass[3].m_preinitalized = true;
	geometryPass[3].m_additional_buffer_flags = (BufferType)0;
	geometryPass[3].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[3].m_ssbo = cullPass->mOutputDrawDataArray;

	std::vector<ShaderBinding> geometryPassFragment(1);
	geometryPassFragment[0].m_type = SHADER_BINDING_COMBINED_TEXTURE_SAMPLER;
	geometryPassFragment[0].m_bindingID = 0;
	geometryPassFragment[0].m_hostvisible = false;
	geometryPassFragment[0].m_useclientbuffer = false;
	geometryPassFragment[0].m_preinitalized = false;
	geometryPassFragment[0].m_additional_buffer_flags = (BufferType)0;
	geometryPassFragment[0].m_shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPassFragment[0].m_sampler.push_back(mSampler);
	geometryPassFragment[0].m_textures.push_back(mWoodTex);
	geometryPassFragment[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	mGeoMaterial = Material_Create(gContext, fbo, &geoConfig, nullptr, { {0, &geometryPass}, {1, &geometryPassFragment } }, {});

	for (int i = 0; i < gFrameOverlapCount; i++) {
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		VkCommandBuffer cmd = mCmds[i];
		vkBeginCommandBuffer(cmd, &beginInfo);

		VkRenderingInfo renderingInfo;
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.pNext = nullptr;
		renderingInfo.flags = 0;
		renderingInfo.renderArea = { {0, 0}, { fbo.m_width, fbo.m_height } };
		renderingInfo.layerCount = 1;
		renderingInfo.viewMask = 0;
		renderingInfo.colorAttachmentCount = 1;

		VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		colorAttachment.imageView = fbo.m_color_attachments[0].GetAttachment()->m_vk_views_per_frame[i];
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		colorAttachment.resolveImageView = nullptr;
		colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue.color = fbo.m_color_attachments[0].m_clear_color;
		VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		depthAttachment.imageView = fbo.m_depth_attachment->GetAttachment()->m_vk_views_per_frame[i];
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

		VkImageMemoryBarrier renderBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		renderBarrier.srcAccessMask = VK_ACCESS_NONE;
		renderBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		renderBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		renderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		renderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		renderBarrier.image = fbo.m_color_attachments[0].GetAttachment()->m_vk_images_per_frame[i];
		renderBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		renderBarrier.subresourceRange.layerCount = 1;
		renderBarrier.subresourceRange.levelCount = 1;

		VkImageMemoryBarrier depthBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		depthBarrier.srcAccessMask = VK_ACCESS_NONE;
		depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		depthBarrier.image = fbo.m_depth_attachment->GetAttachment()->m_vk_images_per_frame[i];
		depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthBarrier.subresourceRange.layerCount = 1;
		depthBarrier.subresourceRange.levelCount = 1;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &renderBarrier);
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthBarrier);

		VkDescriptorSet geometrySets[2] = { mGeoMaterial->m_sets[0]->m_set[i], mGeoMaterial->m_sets[1]->m_set[i] };
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGeoMaterial->m_pipeline_state->m_pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGeoMaterial->m_layout, 0, 2, geometrySets, 0, nullptr);
		vkCmdBindIndexBuffer(cmd, indicesSSBO->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexedIndirect(cmd, cullPass->mOutputDrawDataArray[i]->m_vk_buffer->m_buffer, 0, mECS->GetDrawCount(), sizeof(ShaderTypes::DrawData));

		VkImageMemoryBarrier presentBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		presentBarrier.dstAccessMask = VK_ACCESS_NONE;
		presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		presentBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		presentBarrier.image = fbo.m_color_attachments[0].GetImage(i);
		presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		presentBarrier.subresourceRange.layerCount = 1;
		presentBarrier.subresourceRange.levelCount = 1;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);

		vkCmdEndRenderingKHR(cmd);
		vkEndCommandBuffer(cmd);
	}

}

Application::GeometryPass::~GeometryPass() {
	for (int i = 0; i < gFrameOverlapCount; i++) {
		vkDestroyCommandPool(mDevice, mPools[i], nullptr);
	}
	vkDestroySampler(mDevice, mSampler, nullptr);
	Texture2_Destroy(mWoodTex);
	Texture2_Destroy(mStatueTex);
	Material_Destory(mGeoMaterial);
}

void Application::GeometryPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	mECS->PrepareDataForFrame(FrameIndex);

	glm::mat4 proj = glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->GetWidth(), (float)gWindow->GetHeight(), 0.1f, 1000.0f);

	auto globalDataBuffer = mGeoMaterial->m_sets[0]->GetBuffer2(1, FrameIndex);
	void* ptr = Buffer2_Map(globalDataBuffer);
	ShaderTypes::GlobalData data;
	data.u_DeltaTime = dTime;
	data.u_TimeFromStart = dTimeFromStart;
	data.u_View = mCamera->GetViewMatrix();
	data.u_Projection = proj;
	data.u_ProjView = proj * mCamera->GetViewMatrix();
	memcpy(ptr, &data, sizeof(data));
	Buffer2_Flush(globalDataBuffer, 0, VK_WHOLE_SIZE);
}

VkCommandBuffer Application::GeometryPass::Frame(uint32_t FrameIndex) {
	return mCmds[FrameIndex];
}
