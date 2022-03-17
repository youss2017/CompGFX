#include "Application.hpp"
#include "../physics/Physics.hpp"
#include "../mesh/Map.hpp"
#include "UI.hpp"
#include "Camera.hpp"
#include "ecs/EntityController.hpp"
#include <material_system/Material.hpp>
#include <pipeline/PipelineCS.hpp>
#include <Logger.hpp>
#include <vector>
#include <glfw/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <backend/VkGraphicsCard.hpp>
#include "FrustrumCulling.hpp"

bool Application::Quit = false;

#ifdef _DEBUG
static constexpr bool s_DebugMode = true;
static constexpr const char* s_ShaderCache = "assets/shaders/spirv_cache/";
#else
static constexpr bool s_DebugMode = false;
static constexpr const char* s_ShaderCache = "assets/shaders/spirv_cacheoptimized/";
#endif
static constexpr const char* s_FramebufferReserve = "assets/materials/framebuffer_reserve.cfg";
static constexpr bool s_EnableImGui = true;
static constexpr uint32_t s_MaxObjects = 1;
static constexpr uint32_t s_Range = 10;

vk::VkContext gContext = nullptr;
namespace Application
{
	ConfigurationSettings gConfiguration;
	IGraphics3D gGfx;
	vk::GraphicsSwapchain gSwapchain;
	PlatformWindow* gWindow;
	std::vector<Mesh::Geometry> gGeomtry;
	IBuffer2 gVerticesSSBO, gIndicesBuffer;
	Material* gMaterial0;
	VkSampler gSampler0;
	ITexture2 gWoodTex;
	ITexture2 gStatueTex;
	Camera gCamera({ 0,0,0 });
	Framebuffer gFBO0;
	
	EntityController* gECS;
	static FrustrumCompute frustrumCompute;

}

bool Application::Initalize(ConfigurationSettings* configuration, bool RenderDoc)
{
	PROFILE_FUNCTION();
	log_configure(true, true);
	if (!glfwInit())
	{
		log_error("Could not initalize GLFW, are you missing DLLs");
		return false;
	}
	if (!Graphics3D_CheckVulkanSupport())
	{
		log_error("Vulkan is not supported, try updating your graphics driver.");
		return false;
	}

	if (!Physx_Initalize()) {
		log_error("Could not initalize physx! Are PhysX DLLs in the folder?");
		return false;
	}

	gGfx = Graphics3D_Create(configuration, configuration->WindowTitle.c_str(), s_DebugMode, s_EnableImGui, RenderDoc);
	gContext = ToVKContext(gGfx->m_context);
	gSwapchain = gGfx->m_vswapchain;
	gWindow = gGfx->m_window;

	Shader::ConfigureShaderCache(s_ShaderCache);
	//gFramebufferReserve = new FramebufferReserve(gContext, *configuration, s_FramebufferReserve);

	if (s_EnableImGui)
		UI::Initalize(gContext, gGfx);
	gConfiguration = *configuration;

	return true;
}

bool Application::LoadAssets()
{
	PROFILE_FUNCTION();
	bool MeshLoadStatus = Mesh::LoadVerticesIndicesSSBOs(gContext, 
		{ "assets/mesh/ball.obj" }, gGeomtry, &gVerticesSSBO, &gIndicesBuffer);
	if (!MeshLoadStatus)
		return false;

	//Mesh::LoadVerticesIndicesBONE(gContext, { "assets/mesh/horse.dae" }, skinnedMeshes, &sAnimVertices, &sAnimIndices);

	gECS = new EntityController(gGeomtry);

	srand(140);
	uint32_t instanceCount = 150'000;
	uint32_t instanceSize = instanceCount * sizeof(ShaderTypes::InstanceData);
	ShaderTypes::InstanceData* instance = new ShaderTypes::InstanceData[instanceCount];
	for (int i = 0; i < instanceCount; i++) {
		float x = (rand() % 80) * 5;
		float y = (rand() % 80) * 5;
		float z = (rand() % 80) * 5;
		glm::vec3 offset = glm::vec3(x, y, z + 2);
		offset = offset - (offset / glm::vec3(2.0));
		instance[i].mModel = glm::translate(glm::mat4(1.0), offset);
		instance[i].mTextureID[0] = 0;
	}

	IEntity cube = gECS->GetEntity(EntityGeometryID::ENTITY_GEOMETRY_CUBE);
	cube->m_geometryID = EntityGeometryID::ENTITY_GEOMETRY_CUBE;
	cube->mInstanceCount = instanceCount;
	cube->mInstanceBuffer = Buffer2_CreatePreInitalized(gContext, BUFFER_TYPE_STORAGE, &instance[0], instanceSize, BufferMemoryType::GPU_ONLY, true);
	cube->mCulledInstanceBuffer = Buffer2_Create(gContext, BUFFER_TYPE_STORAGE, instanceSize, BufferMemoryType::GPU_ONLY, true);
	
	return true;
}

bool Application::CreateResources()
{
	PROFILE_FUNCTION();
	gFBO0.m_width = 1280;
	gFBO0.m_height = 720;
	FramebufferAttachment colorAttachment = FramebufferAttachment::Create(gContext, 1280, 720, VK_FORMAT_B10G11R11_UFLOAT_PACK32, { 0.5, 0.5, 0.6, 0 });
	FramebufferAttachment depthAttachment = FramebufferAttachment::Create(gContext, 1280, 720, VK_FORMAT_D32_SFLOAT, { 1.0f, 1.0f, 1.0f, 1.0f });
	gFBO0.AddColorAttachment(0, colorAttachment);
	gFBO0.SetDepthAttachment(depthAttachment);

	MaterialConfiguration basicmc = MaterialConfiguration("assets/materials/basic.mc");

	gSampler0 = vk::Gfx_CreateSampler(gContext);
	gWoodTex = Texture2_CreateFromFile(gContext, "assets/textures/wood.png", true); 
	gStatueTex = Texture2_CreateFromFile(gContext, "assets/textures/statue.jpg", true);
	
	frustrumCompute = CreateFrustrumCompute(gECS->GetGeometryDataArray(), gECS->GetDrawDataArray());

	std::vector<ShaderBinding> geometryPass(4);
	geometryPass[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	geometryPass[0].m_bindingID = 0;
	geometryPass[0].m_hostvisible = false;
	geometryPass[0].m_useclientbuffer = true;
	geometryPass[0].m_preinitalized = false;
	geometryPass[0].m_additional_buffer_flags = (BufferType)0;
	geometryPass[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[0].m_size = 0;
	geometryPass[0].m_client_buffer = gVerticesSSBO;

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
	geometryPass[2].m_ssbo = frustrumCompute.m_outputGeometryDataArray;

	geometryPass[3].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	geometryPass[3].m_bindingID = 3;
	geometryPass[3].m_hostvisible = false;
	geometryPass[3].m_useclientbuffer = false;
	geometryPass[3].m_preinitalized = true;
	geometryPass[3].m_additional_buffer_flags = (BufferType)0;
	geometryPass[3].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	geometryPass[3].m_ssbo = frustrumCompute.m_outputDrawDataArray;

	std::vector<ShaderBinding> geometryPassFragment(1);
	geometryPassFragment[0].m_type = SHADER_BINDING_COMBINED_TEXTURE_SAMPLER;
	geometryPassFragment[0].m_bindingID = 0;
	geometryPassFragment[0].m_hostvisible = false;
	geometryPassFragment[0].m_useclientbuffer = false;
	geometryPassFragment[0].m_preinitalized = false;
	geometryPassFragment[0].m_additional_buffer_flags = (BufferType)0;
	geometryPassFragment[0].m_shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	geometryPassFragment[0].m_sampler.push_back(gSampler0);
	geometryPassFragment[0].m_textures.push_back(gWoodTex);
	geometryPassFragment[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	//geometryPassFragment[0].m_sampler.push_back(gSampler0);
	//geometryPassFragment[0].m_textures.push_back(gStatueTex);
	//geometryPassFragment[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	gMaterial0 = Material_Create(gContext, gFBO0, &basicmc, nullptr, { {0, &geometryPass}, {1, &geometryPassFragment } }, {});
	
	/* Transition Framebuffer Textures into SHADER_READ_ONLY layout which is what the render pass is expecting */
	auto transitionCmd = vk::Gfx_CreateSingleUseCmdBuffer(gContext);
	VkImageMemoryBarrier shaderReadBarrier[gFrameOverlapCount]{};
	for (int i = 0; i < gFrameOverlapCount; i++) {
		shaderReadBarrier[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		shaderReadBarrier[i].srcAccessMask = VK_ACCESS_NONE;
		shaderReadBarrier[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		shaderReadBarrier[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		shaderReadBarrier[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		shaderReadBarrier[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		shaderReadBarrier[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		shaderReadBarrier[i].image = gFBO0.m_color_attachments[0].GetAttachment()->m_vk_images_per_frame[i];
		shaderReadBarrier[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		shaderReadBarrier[i].subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		shaderReadBarrier[i].subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	}
	vkCmdPipelineBarrier(transitionCmd.cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, gFrameOverlapCount, shaderReadBarrier);
	vk::Gfx_SubmitSingleUseCmdBufferAndDestroy(transitionCmd);

	return true;
}

bool Application::Update(double dTimeFromStart, double dTime, double FrameRate, bool UpdateUIInfo)
{
	PROFILE_FUNCTION();
	if (!Graphics3D_PollEvents(gGfx))
		Quit = true;
	/* Camera Controls */
	double RotateRate = 45.0;
	if (gWindow->IsKeyDown('w')) {
		gCamera.MoveForward(dTime * 22.0);
	}if (gWindow->IsKeyDown('s')) {
		gCamera.MoveForward(dTime * -22.0);
	}if (gWindow->IsKeyDown('a')) {
		gCamera.MoveSideways(dTime * -22.0);
	}if (gWindow->IsKeyDown('d')) {
		gCamera.MoveSideways(dTime * 22.0);
	}if (gWindow->IsKeyDown('q')) {
		gCamera.Yaw(dTime * -RotateRate, true);
	}if (gWindow->IsKeyDown('e')) {
		gCamera.Yaw(dTime * RotateRate, true);
	}if (gWindow->IsKeyDown('z')) {
		gCamera.Pitch(dTime * -RotateRate, true);
	}if (gWindow->IsKeyDown('x')) {
		gCamera.Pitch(dTime * RotateRate, true);
	}if (gWindow->IsKeyDown(GLFW_KEY_UP)) {
		gCamera.MoveAlongUpAxis(dTime * -22.0);
	}if (gWindow->IsKeyDown(GLFW_KEY_DOWN)) {
		gCamera.MoveAlongUpAxis(dTime * 22.0);
	}
	auto position = gCamera.GetPosition();
    UI::C_x = position.x, UI::C_y = position.y, UI::C_z = position.z;
	UI::FrameRate = FrameRate;
	if (gWindow->IsKeyUp(GLFW_KEY_ESCAPE))
	{
		Quit = true;
		return false;
	}

	if (UI::StateChanged)
	{
		UI::StateChanged = false;
		auto spec = gMaterial0->m_pipeline_state->m_spec;
		spec.m_PolygonMode = UI::ShowWireframe ? PolygonMode::LINE : PolygonMode::FILL;
		Graphics3D_WaitGPUIdle(gGfx);
		Material_RecreatePipeline(gMaterial0, spec);
	}
	const auto &frame = Graphics3D_GetFrame(gGfx);
	gECS->PrepareDataForFrame(frame.m_FrameIndex);

	glm::mat4 proj = glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->GetWidth(), (float)gWindow->GetHeight(), 0.1f, 1000.0f);

	auto globalDataBuffer = gMaterial0->m_sets[0]->m_bindings[1].m_buffer[frame.m_FrameIndex];
	void* ptr = Buffer2_Map(globalDataBuffer);
	ShaderTypes::GlobalData data;
	data.u_DeltaTime = dTime;
	data.u_TimeFromStart = dTimeFromStart;
	data.u_View = gCamera.GetViewMatrix();
	data.u_Projection = proj;
	data.u_ProjView = proj * gCamera.GetViewMatrix();
	memcpy(ptr, &data, sizeof(data));

	return true;
}

void Application::Render()
{
	PROFILE_FUNCTION();
	VkDevice device = gContext->defaultDevice;
	const auto& frame = Graphics3D_GetFrame(gGfx);
	uint32_t FrameIndex = frame.m_FrameIndex;
	VkCommandBuffer cmd = frame.m_cmd;
	
	gSwapchain.PrepareNextFrame(UINT64_MAX, frame.m_RenderSemaphore, nullptr, nullptr);
	vkWaitForFences(device, 1, &frame.m_RenderFence, true, UINT64_MAX);
	vkResetFences(device, 1, &frame.m_RenderFence);
	vk::Gfx_StartCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	size_t drawBufferSize = gMaterial0->m_sets[0]->m_bindings[3].m_ssbo[FrameIndex]->size;
	VkBuffer inputDrawDataBuffer = gMaterial0->m_sets[0]->m_bindings[3].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer;
	VkBuffer outputDrawDataBuffer = frustrumCompute.m_outputDrawDataArray[FrameIndex]->m_vk_buffer->m_buffer;
	vkCmdFillBuffer(cmd, outputDrawDataBuffer, 0, VK_WHOLE_SIZE, 0);
	VkBufferCopy copyRegion = { 0, 0, drawBufferSize };
	//vkCmdCopyBuffer(cmd, inputDrawDataBuffer, outputDrawDataBuffer, 1, &copyRegion);
	VkBufferMemoryBarrier drawBufferBarrier = vk::Gfx_BufferMemoryBarrier(VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, outputDrawDataBuffer);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &drawBufferBarrier, 0, nullptr);
	VkBufferMemoryBarrier computeVertexShaderBarrier[2] = {
		vk::Gfx_BufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, frustrumCompute.m_outputGeometryDataArray[FrameIndex]->m_vk_buffer->m_buffer),
		vk::Gfx_BufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, frustrumCompute.m_outputDrawDataArray[FrameIndex]->m_vk_buffer->m_buffer)
	};
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 2, computeVertexShaderBarrier, 0, nullptr);

	VkDescriptorSet computeSets[1] = { frustrumCompute.m_set0->m_set[FrameIndex] };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, frustrumCompute.m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, frustrumCompute.m_layout, 0, 1, computeSets, 0, nullptr);
	int drawCount = gECS->GetDrawCount();
	int instanceCount = gECS->GetInstanceCount();

	glm::mat4 projT = glm::transpose(gCamera.GetViewMatrix() * glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->m_width, float(gWindow->m_height), 0.1f, 1000.0f));
	auto normalizePlane = [](glm::vec4 p) -> glm::vec4
	{
		return p / glm::length(glm::vec3(p));
	};
	FrustrumPlanes planes;
	planes.u_MaxGeometry = gGeomtry.size();
	planes.u_CullPlanes[0] = normalizePlane(projT[3] + projT[0]);
	planes.u_CullPlanes[1] = normalizePlane(projT[3] - projT[0]);
	planes.u_CullPlanes[2] = normalizePlane(projT[3] + projT[1]);
	planes.u_CullPlanes[3] = normalizePlane(projT[3] - projT[1]);
	planes.u_CullPlanes[4] = normalizePlane(projT[3] - projT[2]);
	vkCmdPushConstants(cmd, frustrumCompute.m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(planes), &planes);
	vkCmdDispatch(cmd, (instanceCount + 63) / 64, (drawCount + 7) / 8, 1);

	VkRenderingInfo renderingInfo;
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = nullptr;
	renderingInfo.flags = 0;
	renderingInfo.renderArea = { {0, 0}, { gFBO0.m_width, gFBO0.m_height } };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;

	VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	colorAttachment.imageView = gFBO0.m_color_attachments[0].GetAttachment()->m_vk_views_per_frame[FrameIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	colorAttachment.resolveImageView = nullptr;
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color = gFBO0.m_color_attachments[0].m_clear_color;
	VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthAttachment.imageView = gFBO0.m_depth_attachment->GetAttachment()->m_vk_views_per_frame[FrameIndex];
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
	renderBarrier.image = gFBO0.m_color_attachments[0].GetAttachment()->m_vk_images_per_frame[FrameIndex];
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
	depthBarrier.image = gFBO0.m_depth_attachment->GetAttachment()->m_vk_images_per_frame[FrameIndex];
	depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthBarrier.subresourceRange.layerCount = 1;
	depthBarrier.subresourceRange.levelCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &renderBarrier);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthBarrier);
	
	VkDescriptorSet geometrySets[2] = { gMaterial0->m_sets[0]->m_set[FrameIndex], gMaterial0->m_sets[1]->m_set[FrameIndex] };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial0->m_pipeline_state->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial0->m_layout, 0, 2, geometrySets, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, gIndicesBuffer->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirect(cmd, frustrumCompute.m_outputDrawDataArray[FrameIndex]->m_vk_buffer->m_buffer, 0, gECS->GetDrawCount(), sizeof(ShaderTypes::DrawData));

	VkImageMemoryBarrier presentBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	presentBarrier.dstAccessMask = VK_ACCESS_NONE;
	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	presentBarrier.image = gFBO0.m_color_attachments[0].GetImage(FrameIndex);
	presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentBarrier.subresourceRange.layerCount = 1;
	presentBarrier.subresourceRange.levelCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);

	vkCmdEndRenderingKHR(cmd);
	vkEndCommandBuffer(cmd);

	VkPipelineStageFlags stage[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.m_RenderSemaphore;
	submitInfo.pWaitDstStageMask = stage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.m_PresentSemaphore;
	vkQueueSubmit(gContext->defaultQueue, 1, &submitInfo, frame.m_RenderFence);

	if (s_EnableImGui)
		UI::RenderUI();
	gSwapchain.Present(gFBO0.m_color_attachments[0].GetImage(FrameIndex), gFBO0.m_color_attachments[0].GetView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, UI::ShowDepthBuffer);
	Graphics3D_NextFrame(gGfx);
}

void Application::Destroy()
{
	PROFILE_FUNCTION();
	Graphics3D_WaitGPUIdle(gGfx);
	DestroyFrusturumCompute(frustrumCompute);
	Material_Destory(gMaterial0);
	Buffer2_Destroy(gECS->GetEntity(EntityGeometryID::ENTITY_GEOMETRY_CUBE)->mInstanceBuffer);
	Buffer2_Destroy(gECS->GetEntity(EntityGeometryID::ENTITY_GEOMETRY_CUBE)->mCulledInstanceBuffer);
	gFBO0.DestroyAllBoundAttachments();
	vkDestroySampler(gContext->defaultDevice, gSampler0, nullptr);
	Texture2_Destroy(gWoodTex);
	Texture2_Destroy(gStatueTex);
	Buffer2_Destroy(gVerticesSSBO);
	Buffer2_Destroy(gIndicesBuffer);
	Physx_Destroy();
	delete gECS;
	Graphics3D_Destroy(gGfx);
}
