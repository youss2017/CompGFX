#include "Application.hpp"
#include "../physics/Physics.hpp"
#include "UI.hpp"
#include "Camera.hpp"
#include "ecs/EntityController.hpp"
#include <material_system/FramebufferReserve.hpp>
#include <material_system/Material.hpp>
#include <pipeline/PipelineCS.hpp>
#include <Logger.hpp>
#include <vector>
#include <glfw/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

bool Application::Quit = false;

#ifdef _DEBUG
static constexpr bool s_DebugMode = true;
#else
static constexpr bool s_DebugMode = false;
#endif
static constexpr const char* s_ShaderCache = "assets/materials/shaders/spirv_cache/";
static constexpr const char* s_FramebufferReserve = "assets/materials/framebuffer_reserve.cfg";
static constexpr bool s_EnableImGui = true;
static constexpr uint32_t s_MaxObjects = 500'000;

namespace Application
{
	ConfigurationSettings gConfiguration;
	IGraphics3D gGfx;
	vk::VkContext gContext = nullptr;
	vk::GraphicsSwapchain gSwapchain;
	PlatformWindow* gWindow;
	FramebufferReserve* gFramebufferReserve = nullptr;
	std::vector<Mesh::Geometry> gGeomtry;
	IBuffer2 gVerticesSSBO, gIndicesBuffer;
	IFramebufferStateManagement gFBOStateManagment0;
	IMaterialFramebuffer gFBO0;
	Material* gMaterial0;
	VkSampler gSampler0;
	ITexture2 gWoodTex;
	Camera gCamera({0,0,0});
	
	static struct {
		uint32_t count;
		Entity* ent;
	} s_Entites;

	EntityController* gECS;

	static VkDescriptorPool computeSetPool;
	static ShaderSet computeSet0;
	static VkPipelineLayout computeLayout;
	static VkPipeline computeFrustrum;

	// Shader Data
	// !!! IMPORTANT !!! Write ONLY by using memcpy
	// using the '=' operator will cause a read operation, since were using coherent memory
	// that means read from GPU memory to CPU memory
	static ShaderTypes::SceneData* s_ScenePtr[gFrameOverlapCount];
	ShaderTypes::ObjectData* gObjectData[gFrameOverlapCount];
	ShaderTypes::DrawData* gDraws[gFrameOverlapCount];

}

bool Application::Initalize(ConfigurationSettings* configuration)
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

	gGfx = Graphics3D_Create(configuration, configuration->WindowTitle.c_str(), s_DebugMode, s_EnableImGui);
	gContext = ToVKContext(gGfx->m_context);
	gSwapchain = gGfx->m_vswapchain;
	gWindow = gGfx->m_window;

	Shader::ConfigureShaderCache(s_ShaderCache);
	gFramebufferReserve = new FramebufferReserve(gContext, *configuration, s_FramebufferReserve);

	if (s_EnableImGui)
		UI::Initalize(gContext, gGfx);
	gConfiguration = *configuration;

	return true;
}

bool Application::LoadAssets()
{
	PROFILE_FUNCTION();
	bool MeshLoadStatus = Mesh::LoadVerticesIndicesSSBOs(gContext, 
		{ "assets/mesh/cube.obj",
		 "assets/mesh/ball.obj"
		}, gGeomtry, &gVerticesSSBO, &gIndicesBuffer);
	if (!MeshLoadStatus)
		return false;

	gECS = new EntityController(gGeomtry, s_MaxObjects, s_MaxObjects);

	s_Entites.count = s_MaxObjects;
	s_Entites.ent = new Entity[s_Entites.count];
	
	srand(42);
	int range = 450;
	for (unsigned int i = 0; i < s_Entites.count; i++)
	{
		s_Entites.ent[i].m_geometryID = EntityGeometryID(rand() % 2);
		s_Entites.ent[i].m_objData.bounding_sphere_center = glm::vec3(0.0);
		s_Entites.ent[i].m_objData.bounding_sphere_radius = 1.0;
		s_Entites.ent[i].m_objData.m_Model = glm::translate(glm::mat4(1.0), glm::vec3((rand() % range) - (range / 2), (rand() % range) - (range / 2), (rand() % range) - (range / 2)));
		s_Entites.ent[i].m_objData.m_NormalModel = glm::mat4(1.0);
		s_Entites.ent[i].m_textureID = 0;
		gECS->AddEntity(&s_Entites.ent[i]);
	}

	return true;
}

bool Application::CreateResources()
{
	PROFILE_FUNCTION();
	MaterialConfiguration basicmc = MaterialConfiguration("assets/materials/basic.mc");
	gFBOStateManagment0 = Material_CreateFramebufferStateManagment(gContext, &basicmc, gFramebufferReserve);

	gSampler0 = vk::Gfx_CreateSampler(gContext);
	gWoodTex = Texture2_CreateFromFile(gContext, "assets/textures/wood.png", true);
	
	std::vector<ShaderBinding> bindings(4);
	bindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[0].m_bindingID = 0;
	bindings[0].m_hostvisible = true;
	bindings[0].m_useclientbuffer = true;
	bindings[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].m_size = 0;
	bindings[0].m_client_buffer = gVerticesSSBO;
	
	bindings[1] = bindings[0];
	bindings[1].m_useclientbuffer = false;
	bindings[1].m_type = SHADER_BINDING_UNIFORM_BUFFER;
	bindings[1].m_bindingID = 1;
	bindings[1].m_size = sizeof(ShaderTypes::SceneData);
	
	bindings[2] = bindings[1];
	bindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[2].m_bindingID = 2;
	bindings[2].m_size = s_MaxObjects * sizeof(ShaderTypes::ObjectData);
	
	bindings[3] = bindings[2];
	bindings[3].m_bindingID = 3;
	bindings[3].m_additional_buffer_flags = BufferType::IndirectBuffer;
	bindings[3].m_size = s_MaxObjects * sizeof(ShaderTypes::DrawData);
	
	std::vector<ShaderBinding> set1Bindings(1);
	set1Bindings[0].m_type = SHADER_BINDING_COMBINED_TEXTURE_SAMPLER;
	set1Bindings[0].m_bindingID = 0;
	set1Bindings[0].m_hostvisible = false;
	set1Bindings[0].m_useclientbuffer = false;
	set1Bindings[0].m_shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	set1Bindings[0].m_size = 0;
	set1Bindings[0].m_sampler.push_back(gSampler0);
	set1Bindings[0].m_textures.push_back(gWoodTex);
	set1Bindings[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	gMaterial0 = Material_Create(gContext, &basicmc, gFBOStateManagment0, nullptr, { {0, &bindings}, {1, &set1Bindings} }, {});
	gFBO0 = Material_CreateFramebuffer(gContext, &basicmc, gConfiguration, gFBOStateManagment0, gFramebufferReserve);

	for (int i = 0; i < gFrameOverlapCount; i++)
	{
		s_ScenePtr[i] = (ShaderTypes::SceneData*)Buffer2_Map(bindings[1].m_buffer[i]);
		gObjectData[i] = (ShaderTypes::ObjectData*)Buffer2_Map(bindings[2].m_ssbo[i]);
		gDraws[i] = (ShaderTypes::DrawData*)Buffer2_Map(bindings[3].m_ssbo[i]);
		gECS->UpdateDrawCommandAndObjectDataBuffer(gDraws[i], gObjectData[i]);
	}

	std::vector<ShaderBinding> computeBindings(2);
	computeBindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	computeBindings[0].m_bindingID = 0;
	computeBindings[0].m_hostvisible = true;
	computeBindings[0].m_useclientbuffer = false;
	computeBindings[0].m_preinitalized = true;
	computeBindings[0].m_additional_buffer_flags = (BufferType)0;
	computeBindings[0].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[0].m_ssbo = bindings[2].m_ssbo;

	computeBindings[1].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	computeBindings[1].m_bindingID = 1;
	computeBindings[1].m_hostvisible = true;
	computeBindings[1].m_useclientbuffer = false;
	computeBindings[1].m_preinitalized = true;
	computeBindings[1].m_additional_buffer_flags = (BufferType)0;
	computeBindings[1].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[1].m_ssbo = bindings[3].m_ssbo;

	std::vector<VkDescriptorPoolSize> poolSize;
	ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSize, &computeBindings);
	computeSetPool = vk::Gfx_CreateDescriptorPool(gContext, 1 * gFrameOverlapCount, poolSize);
	computeSet0 = ShaderBinding_Create(gContext, computeSetPool, 0, &computeBindings);
	struct FrustrumPlanes
	{
		glm::vec4 u_CullPlanes[6];
	};
	computeLayout = ShaderBinding_CreatePipelineLayout(gContext, { computeSet0 }, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FrustrumPlanes)}});
	Shader computeShader = Shader(gContext, "assets/materials/shaders/FrustrumCulling.comp");
	computeFrustrum = Pipeline_CreateCompute(gContext, &computeShader, computeLayout, 0);

	return true;
}

bool Application::Update(double dTime, double FrameRate)
{
	PROFILE_FUNCTION();
	if (!Graphics3D_PollEvents(gGfx))
		Quit = true;
	/* Camera Controls */
	double RotateRate = 45.0;
	if (gWindow->IsKeyDown('w')) {
		gCamera.MoveForward(dTime * -22.0);
	}if (gWindow->IsKeyDown('s')) {
		gCamera.MoveForward(dTime * 22.0);
	}if (gWindow->IsKeyDown('a')) {
		gCamera.MoveSideways(dTime * -22.0);
	}if (gWindow->IsKeyDown('d')) {
		gCamera.MoveSideways(dTime * 22.0);
	}if (gWindow->IsKeyDown('q')) {
		gCamera.Yaw(dTime * RotateRate, true);
	}if (gWindow->IsKeyDown('e')) {
		gCamera.Yaw(dTime * -RotateRate, true);
	}if (gWindow->IsKeyDown('z')) {
		gCamera.Pitch(dTime * RotateRate, true);
	}if (gWindow->IsKeyDown('x')) {
		gCamera.Pitch(dTime * -RotateRate, true);
	}if (gWindow->IsKeyDown(GLFW_KEY_UP)) {
		gCamera.MoveAlongUpAxis(dTime * -22.0);
	}if (gWindow->IsKeyDown(GLFW_KEY_DOWN)) {
		gCamera.MoveAlongUpAxis(dTime * 22.0);
	}
	auto position = gCamera.GetPosition();
    UI::C_x = position.x, UI::C_y = position.y, UI::C_z = position.z;
	if (gWindow->IsKeyUp(GLFW_KEY_ESCAPE))
	{
		Quit = true;
		return false;
	}
	UI::FrameRate = FrameRate;
	UI::FrameTime = dTime * 1e3;

	const auto &frame = Graphics3D_GetFrame(gGfx);

	glm::mat4 proj = glm::perspective(90.0f, gWindow->m_width / float(gWindow->m_height), 0.1f, 1000.0f);
	glm::mat4 view = gCamera.GetViewMatrix();
	memcpy(&s_ScenePtr[frame.m_FrameIndex]->m_Projection, &proj, sizeof(glm::mat4));
	memcpy(&s_ScenePtr[frame.m_FrameIndex]->m_View, &view, sizeof(glm::mat4));
	
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
	auto& attachments = gMaterial0->m_state_managment->m_attachments;
	VkClearValue pClearValues[2];
	pClearValues[0] = attachments[0].clearColor;
	pClearValues[1] = gMaterial0->m_state_managment->m_depth_attachment.value().clearColor;
	VkRenderPassBeginInfo BeginInfo;
	VkViewport viewport;
	VkRect2D scissor;
	BeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	BeginInfo.pNext = nullptr;
	BeginInfo.renderPass = gMaterial0->m_state_managment->m_renderpass;
	BeginInfo.framebuffer = gFBO0->m_framebuffers[frame.m_FrameIndex];
	gFBO0->m_framebuffer->GetViewport(BeginInfo.renderArea.offset.x, BeginInfo.renderArea.offset.y, BeginInfo.renderArea.extent.width, BeginInfo.renderArea.extent.height);
	gFBO0->m_framebuffer->GetViewport(viewport.x, viewport.y, viewport.width, viewport.height);
	gFBO0->m_framebuffer->GetScissor(scissor.offset.x, scissor.offset.y, scissor.extent.width, scissor.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	BeginInfo.clearValueCount = 2;
	BeginInfo.pClearValues = pClearValues;
	
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computeFrustrum);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computeLayout, 0, 1, &computeSet0->m_set[FrameIndex], 0, nullptr);
	struct {
		glm::vec4 u_FrustrumPlanes[6];
	} computePushblock;

	glm::mat4 proj = glm::transpose(glm::perspective(90.0f, gWindow->m_width / float(gWindow->m_height), 0.1f, 1000.0f));
	computePushblock.u_FrustrumPlanes[0] = proj[3] + proj[0];
	computePushblock.u_FrustrumPlanes[1] = proj[3] - proj[0];
	computePushblock.u_FrustrumPlanes[2] = proj[3] + proj[1];
	computePushblock.u_FrustrumPlanes[3] = proj[3] - proj[1];
	computePushblock.u_FrustrumPlanes[4] = proj[3] - proj[2];
	computePushblock.u_FrustrumPlanes[5] = glm::vec4(0.0);

	vkCmdPushConstants(cmd, computeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(computePushblock), &computePushblock);
	int temp = (gECS->GetDrawCount() + 31) / 32;
	vkCmdDispatch(cmd, temp, 1, 1);

	VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
	barrier.srcQueueFamilyIndex = gContext->defaultQueueFamilyIndex;
	barrier.dstQueueFamilyIndex = gContext->defaultQueueFamilyIndex;
	barrier.buffer = computeSet0->m_bindings[1].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer;
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &barrier, 0, nullptr);

	vkCmdBeginRenderPass(cmd, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial0->m_pipeline_state->m_pipeline);
	VkDescriptorSet sets[2] = { gMaterial0->m_sets[0]->m_set[FrameIndex], gMaterial0->m_sets[1]->m_set[FrameIndex]};
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial0->m_layout, 0, 2, sets, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, gIndicesBuffer->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexedIndirect(cmd, gMaterial0->m_sets[0]->m_bindings[3].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer, 0, s_Entites.count, sizeof(ShaderTypes::DrawData));
	
	vkCmdEndRenderPass(cmd);
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
	gSwapchain.Present(gFBO0->m_textures[UI::ShowDepthBuffer]->m_vk_images_per_frame[FrameIndex], gFBO0->m_textures[UI::ShowDepthBuffer]->m_vk_views_per_frame[FrameIndex], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, UI::ShowDepthBuffer);
	Graphics3D_NextFrame(gGfx);
}

void Application::Destroy()
{
	PROFILE_FUNCTION();
	Graphics3D_WaitGPUIdle(gGfx);
	vkDestroyDescriptorPool(gContext->defaultDevice, computeSetPool, nullptr);
	ShaderBinding_DestroySets(gContext, { computeSet0 });
	vkDestroyPipelineLayout(gContext->defaultDevice, computeLayout, nullptr);
	vkDestroyPipeline(gContext->defaultDevice, computeFrustrum, nullptr);
	FramebufferStateManagment_Destroy(gFBOStateManagment0);
	Material_DestroyFramebuffer(gFBO0);
	Material_Destory(gMaterial0);
	vkDestroySampler(gContext->defaultDevice, gSampler0, nullptr);
	Texture2_Destroy(gWoodTex);
	Buffer2_Destroy(gVerticesSSBO);
	Buffer2_Destroy(gIndicesBuffer);
	delete gFramebufferReserve;
	Graphics3D_Destroy(gGfx);
	Physx_Destroy();
	delete[] s_Entites.ent;
	delete gECS;
}
