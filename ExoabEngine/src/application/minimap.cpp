#include "minimap.hpp"
#include "Globals.hpp"
#include "pipeline/Pipeline.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image_write.h>
#include "Camera.hpp"

ITexture2 Application::GenerateMinimap(Terrain* terrain, std::vector<ITexture2> terrainTextures, ITexture2 normalMap)
{
	auto transform = terrain->GetTransform();
	terrain->SetTransform(glm::scale(glm::mat4(1.0), glm::vec3(0.10)) * terrain->GetToCenterTransform());
	Shader vertex = Shader(Global::Context, "assets/shaders/terrain/minimap.vert");
	Shader fragment = Shader(Global::Context, "assets/shaders/terrain/minimap.frag");
	Framebuffer fbo;
	fbo.m_width = 256;
	fbo.m_height = 256;
	FramebufferAttachment colorAttachment = FramebufferAttachment::Create(Global::Context, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, fbo.m_width, fbo.m_height, VK_FORMAT_R8G8B8A8_UNORM, { 0.0 });
	FramebufferAttachment depthAttachment = FramebufferAttachment::Create(Global::Context, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, fbo.m_width, fbo.m_height, VK_FORMAT_D32_SFLOAT, { 1.0 });

	auto cmd = vk::Gfx_CreateSingleUseCmdBuffer(Global::Context);
	vk::Framebuffer_TransistionAttachment(cmd.cmd, &colorAttachment, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vk::Framebuffer_TransistionAttachment(cmd.cmd, &depthAttachment, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	fbo.AddColorAttachment(0, colorAttachment);
	fbo.SetDepthAttachment(depthAttachment);

	BindingDescription bindings[4]{};
	bindings[0].mBindingID = 0;
	bindings[0].mFlags;
	bindings[0].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].mStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].mBufferSize = 0;
	bindings[0].mBuffer = terrain->GetVerticesBuffer();
	bindings[0].mSharedResources = true;

	bindings[1].mBindingID = 1;
	bindings[1].mFlags = BINDING_FLAG_CPU_VISIBLE;
	bindings[1].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[1].mStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].mBufferSize = sizeof(ShaderTypes::GlobalData);
	bindings[1].mBuffer = nullptr;
	bindings[1].mSharedResources = false;

	bindings[2].mBindingID = 2;
	bindings[2].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[2].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	for (auto texture : terrainTextures)
		bindings[2].mTextures.push_back(texture);
	bindings[2].mSampler = Global::DefaultSampler;

	bindings[3].mBindingID = 3;
	bindings[3].mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[3].mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[3].mTextures.push_back(normalMap);
	bindings[3].mSampler = Global::DefaultSampler;

	std::vector<VkDescriptorPoolSize> poolSizes;
	ShaderConnector_CalculateDescriptorPool(4, bindings, poolSizes);
	VkDescriptorPool pool = vk::Gfx_CreateDescriptorPool(Global::Context, 3, poolSizes);
	DescriptorSet set = ShaderConnector_CreateSet(Global::Context, 0, pool, 4, bindings);
	struct {
		glm::mat4 u_Model;
		glm::mat3 u_NormalModel;
	} pushblock;
	pushblock.u_Model = terrain->GetTransform();
	pushblock.u_NormalModel = glm::mat3(glm::transpose(glm::inverse(pushblock.u_Model)));
	VkPipelineLayout layout = ShaderConnector_CreatePipelineLayout(Global::Context, 1, &set, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushblock)} });

	ShaderTypes::GlobalData* data = (ShaderTypes::GlobalData*)Buffer2_Map(set.GetBuffer2(1));
	memset(data, 0, sizeof(ShaderTypes::GlobalData));
	data->u_LightDirection = glm::vec4(glm::normalize(glm::vec3(0.0, -100.0, 0.0) - glm::vec3(0.0)), 0.0);
	data->u_Projection = Global::Projection;
	Camera camera;
	camera.SetPosition(glm::vec3(0.0, 10.0, 0.0));
	camera.Pitch(90.0, true, true);
	camera.UpdateViewMatrix();
	data->u_View = camera.GetViewMatrix();
	data->u_ProjView = data->u_Projection * camera.GetViewMatrix();
	Buffer2_Flush(set.GetBuffer2(1), 0, VK_WHOLE_SIZE);

	PipelineSpecification spec;
	spec.m_CullMode = CullMode::CULL_NONE;
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
	auto state = PipelineState_Create(Global::Context, spec, input, fbo, layout, &vertex, &fragment);

	VkRenderingInfoKHR renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
	renderingInfo.renderArea.extent = {fbo.m_width, fbo.m_height};
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;

	VkRenderingAttachmentInfo colorInfo = fbo.GetRenderingAttachmentInfo(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
	VkRenderingAttachmentInfo depthInfo = fbo.GetDepthRenderingAttachmentInfo(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

	renderingInfo.pColorAttachments = &colorInfo;
	renderingInfo.pDepthAttachment = &depthInfo;

	vkCmdBeginRenderingKHR(cmd.cmd, &renderingInfo);

	vkCmdBindPipeline(cmd.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->m_pipeline);
	vkCmdBindDescriptorSets(cmd.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set[0], 0, nullptr);
	vkCmdPushConstants(cmd.cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushblock), &pushblock);

	vkCmdBindIndexBuffer(cmd.cmd, terrain->GetIndicesBuffer()->mBuffers[0], 0, VK_INDEX_TYPE_UINT32);
	for (uint32_t i = 0; i < terrain->GetSubmeshCount(); i++) {
		auto& submesh = terrain->GetSubmesh(i);
		vkCmdDrawIndexed(cmd.cmd, submesh.mIndicesCount, 1, submesh.mFirstIndex, submesh.mFirstVertex, 0);
	}

	vk::Framebuffer_TransistionImage(cmd.cmd, colorAttachment.GetAttachment(), VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	vkCmdEndRenderingKHR(cmd.cmd);

	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(cmd);
	
	void* pixelBuffer = malloc(sizeof(int) * fbo.m_width * fbo.m_height);
	Texture2_ReadPixels(colorAttachment.GetAttachment(), VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sizeof(int), pixelBuffer);
	fbo.DestroyAllBoundAttachments();

	vkDestroyPipelineLayout(Global::Context->defaultDevice, layout, nullptr);
	vkDestroyDescriptorPool(Global::Context->defaultDevice, pool, nullptr);
	ShaderConnector_DestroySet(Global::Context->defaultDevice, set);
	PipelineState_Destroy(state);

	terrain->SetTransform(transform);

	Texture2DSpecification tspec;
	tspec.m_Width = fbo.m_width;
	tspec.m_Height = fbo.m_height;
	tspec.mLayers = 1;
	tspec.m_TextureUsage = TextureUsage::TEXTURE;
	tspec.m_Samples = TextureSamples::MSAA_1;
	tspec.m_Format = VK_FORMAT_R8G8B8A8_UNORM;
	tspec.m_GenerateMipMapLevels = false;
	ITexture2 minimap = Texture2_Create(Global::Context, tspec);
	Texture2_UploadPixels(minimap, pixelBuffer, sizeof(int)* fbo.m_width* fbo.m_height);
	free(pixelBuffer);
	return minimap;
}
