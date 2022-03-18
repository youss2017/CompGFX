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
#include "scene/FrustrumCullPass.hpp"
#include "scene/GeometryPass.hpp"

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
	Camera gCamera({ 0,0,0 });
	Camera gLockedCamera({ 0,0,0 });
	Framebuffer gFBO0;
	
	EntityController* gECS;
	static FrustumCullPass* cullPass;
	static GeometryPass* geoPass;
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

	if (s_EnableImGui)
		UI::Initalize(gContext, gGfx);
	gConfiguration = *configuration;

	return true;
}

bool Application::LoadAssets()
{
	PROFILE_FUNCTION();
	bool MeshLoadStatus = Mesh::LoadVerticesIndicesSSBOs(gContext, 
		{ "assets/mesh/lucy.fbx" }, gGeomtry, &gVerticesSSBO, &gIndicesBuffer);
	if (!MeshLoadStatus)
		return false;

	gECS = new EntityController(gGeomtry);

	srand(140);
	uint32_t instanceCount = 100;
	uint32_t instanceSize = instanceCount * sizeof(ShaderTypes::InstanceData);
	ShaderTypes::InstanceData* instance = new ShaderTypes::InstanceData[instanceCount];
	for (unsigned int i = 0; i < instanceCount; i++) {
		float x = (rand() % 1000) * 26;
		float y = (rand() % 1000) * 26;
		float z = (rand() % 1000) * 26;
		glm::vec3 offset = glm::vec3(x, y, z + 2);
		offset = offset - (offset / glm::vec3(2.0));
		instance[i].mModel = glm::translate(glm::scale(glm::mat4(1.0), glm::vec3(1e-3)), offset);
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

	MaterialConfiguration geometryPassConfig = MaterialConfiguration("assets/materials/geometryPass.mc");

	cullPass = new FrustumCullPass(gECS, &gLockedCamera);
	geoPass = new GeometryPass(gVerticesSSBO, gIndicesBuffer, geometryPassConfig, gFBO0, cullPass, &gCamera, gECS);

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
		Framebuffer_TransistionAttachment(transitionCmd.cmd, &gFBO0.m_depth_attachment.value(), VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED, 0, 1, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
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
	if (gWindow->IsKeyUp(GLFW_KEY_ESCAPE))
	{
		Quit = true;
		return false;
	}

	if (UI::StateChanged)
	{
		UI::StateChanged = false;
		geoPass->SetWireframeMode(UI::ShowWireframe);
	}
	if (!UI::LockFrustrum) {
		memcpy(&gLockedCamera, &gCamera, sizeof(Camera));
	}

	const auto &frame = Graphics3D_GetFrame(gGfx);
	if (UpdateUIInfo) {
		UI::FrameRate = FrameRate;
		UI::FrameTime = dTime * 1e3;
		cullPass->GetComputeShaderStatistics(frame.m_FrameIndex, false, UI::FrustrumCullingTime, UI::FrustrumInvocations);
	}

	cullPass->Prepare(frame.m_FrameIndex, dTime, dTimeFromStart);
	geoPass->Prepare(frame.m_FrameIndex, dTime, dTimeFromStart);

	return true;
}

void Application::Render()
{
	PROFILE_FUNCTION();
	VkDevice device = gContext->defaultDevice;
	const auto& frame = Graphics3D_GetFrame(gGfx);
	uint32_t FrameIndex = frame.m_FrameIndex;
	
	gSwapchain.PrepareNextFrame(UINT64_MAX, frame.m_RenderSemaphore, nullptr, nullptr);
	vkWaitForFences(device, 1, &frame.m_RenderFence, true, UINT64_MAX);
	vkResetFences(device, 1, &frame.m_RenderFence);
	
	VkCommandBuffer cmds[2] = { cullPass->Frame(FrameIndex), geoPass->Frame(FrameIndex) };

	VkPipelineStageFlags stage[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.m_RenderSemaphore;
	submitInfo.pWaitDstStageMask = stage;
	submitInfo.commandBufferCount = 2;
	submitInfo.pCommandBuffers = cmds;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.m_PresentSemaphore;
	vkQueueSubmit(gContext->defaultQueue, 1, &submitInfo, frame.m_RenderFence);

	if (s_EnableImGui)
		UI::RenderUI();
	if (UI::ShowDepthBuffer) {
		gSwapchain.Present(gFBO0.m_depth_attachment.value().GetImage(FrameIndex), gFBO0.m_depth_attachment.value().GetView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, UI::ShowDepthBuffer);
	}
	else {
		gSwapchain.Present(gFBO0.m_color_attachments[0].GetImage(FrameIndex), gFBO0.m_color_attachments[0].GetView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, UI::ShowDepthBuffer);
	}
	Graphics3D_NextFrame(gGfx);
}

void Application::Destroy()
{
	PROFILE_FUNCTION();
	Graphics3D_WaitGPUIdle(gGfx);
	delete cullPass;
	delete geoPass;
	Buffer2_Destroy(gECS->GetEntity(EntityGeometryID::ENTITY_GEOMETRY_CUBE)->mInstanceBuffer);
	Buffer2_Destroy(gECS->GetEntity(EntityGeometryID::ENTITY_GEOMETRY_CUBE)->mCulledInstanceBuffer);
	gFBO0.DestroyAllBoundAttachments();
	Buffer2_Destroy(gVerticesSSBO);
	Buffer2_Destroy(gIndicesBuffer);
	Physx_Destroy();
	delete gECS;
	Graphics3D_Destroy(gGfx);
}
