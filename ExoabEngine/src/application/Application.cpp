#include "Application.hpp"
#include "../physics/Physics.hpp"
#include "UI.hpp"
#include "Camera.hpp"
#include "ecs/EntityController.hpp"
#include <pipeline/PipelineCS.hpp>
#include <Logger.hpp>
#include <vector>
#include <glfw/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <backend/VkGraphicsCard.hpp>
#include "scene/FrustrumCullPass.hpp"
#include "scene/GeometryPass.hpp"
#include "scene/SkyboxPass.hpp"
#include "scene/ShadowPass.hpp"
#include "scene/DebugPass.hpp"
#include "scene/postprocess/BloomPass.hpp"

bool Application::Quit = false;

#ifdef _DEBUG
static constexpr bool s_DebugMode = true;
static constexpr const char* s_ShaderCache = "assets/shaders/spirv_cache/";
#else
static constexpr bool s_DebugMode = false;
static constexpr const char* s_ShaderCache = "assets/shaders/spirv_cacheoptimized/";
#endif
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
	static SkyboxPass* skybox;
	static ShadowPass* shadow;
	static DebugPass* debugPass;
	static BloomPass* bloom;
	static VkSemaphore shadowPassSemaphore[gFrameOverlapCount];
	static VkSemaphore bloomPassSemaphore[gFrameOverlapCount];
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
	Mesh::GeometryConfiguration config;
	config.Load("assets/geometry.cfg");
	bool MeshLoadStatus = Mesh::LoadVerticesIndicesSSBOs(gContext, config, gGeomtry, &gVerticesSSBO, &gIndicesBuffer);
	if (!MeshLoadStatus)
		return false;

	gECS = new EntityController(gGeomtry);

	srand(140);
	uint32_t instanceCount = 500;
	uint32_t instanceSize = instanceCount * sizeof(ShaderTypes::InstanceData);
	ShaderTypes::InstanceData* instance = new ShaderTypes::InstanceData[instanceCount];
	for (unsigned int i = 0; i < instanceCount; i++) {
		float x = (rand() % 25) * 5;
		float y = (rand() % 25) * 5;
		float z = (rand() % 25) * 5;
		glm::vec3 offset = glm::vec3(x, y, z + 2);
		offset = offset - (offset / glm::vec3(2.0));
		instance[i].mModel = glm::translate(glm::scale(glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0)), glm::vec3(1)), offset);
		instance[i].mTextureID[0] = 0;
	}

	IEntity cube = gECS->GetEntity(EntityGeometryID::ENTITY_GEOMETRY_CUBE);
	cube->m_geometryID = EntityGeometryID::ENTITY_GEOMETRY_CUBE;
	cube->mInstanceCount = instanceCount;
	cube->mInstanceBuffer = Buffer2_CreatePreInitalized(BUFFER_TYPE_STORAGE, &instance[0], instanceSize, BufferMemoryType::CPU_TO_GPU, true, false);
	cube->mCulledInstanceBuffer = Buffer2_Create(BUFFER_TYPE_STORAGE, instanceSize, BufferMemoryType::GPU_ONLY, true, false);
	
	return true;
}

bool Application::CreateResources()
{
	PROFILE_FUNCTION();
	gFBO0.m_width = 1280;
	gFBO0.m_height = 720;
	FramebufferAttachment colorAttachment = FramebufferAttachment::Create(gContext, VK_IMAGE_USAGE_STORAGE_BIT, 1280, 720, VK_FORMAT_B10G11R11_UFLOAT_PACK32, { 0.5, 0.5, 0.6, 0 });
	FramebufferAttachment depthAttachment = FramebufferAttachment::Create(gContext, 0, 1280, 720, VK_FORMAT_D32_SFLOAT, { 1.0f, 1.0f, 1.0f, 1.0f });
	gFBO0.AddColorAttachment(0, colorAttachment);
	gFBO0.SetDepthAttachment(depthAttachment);

	shadow = new ShadowPass(gVerticesSSBO, gIndicesBuffer, gECS, &gCamera, 1024);
	cullPass = new FrustumCullPass(gECS, &gLockedCamera);
	geoPass = new GeometryPass(gVerticesSSBO, gIndicesBuffer, gFBO0, cullPass, &gCamera, gECS, shadow->GetDepthAttachment());
	skybox = new SkyboxPass("assets/textures/cubemap4.png", geoPass, &gCamera, true);
	debugPass = new DebugPass(gFBO0);
	bloom = new BloomPass(gFBO0, 0, 1.0);
	UI::CubemapLODMax = skybox->GetMaxLOD();

	for (int i = 0; i < gFrameOverlapCount; i++) {
		shadowPassSemaphore[i] = vk::Gfx_CreateSemaphore(gContext, false);
		bloomPassSemaphore[i] = vk::Gfx_CreateSemaphore(gContext, false);
	}

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
		Framebuffer_TransistionAttachment(transitionCmd.cmd, &gFBO0.m_depth_attachment.value(), VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED);
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
		Graphics3D_WaitGPUIdle(gGfx);
		UI::StateChanged = false;
		if (UI::ClearShaderCache) {
			UI::ClearShaderCache = false;
			logalert("Deleting Shader Cache.");
			std::filesystem::remove_all(s_ShaderCache);
		}
		if (UI::ReloadShaders) {
			UI::ReloadShaders = false;
			logalert("Reloading ALL Scene Shaders.");
			cullPass->ReloadShaders();
			geoPass->ReloadShaders();
			skybox->ReloadShaders();
			shadow->ReloadShaders();
			debugPass->ReloadShaders();
			bloom->ReloadShaders();
		}
		geoPass->SetWireframeMode(UI::ShowWireframe);
		//Graphics3D_SetSyncInterval(gGfx, UI::VSync);
	}
	if (!UI::LockFrustrum) {
		memcpy(&gLockedCamera, &gCamera, sizeof(Camera));
	}

	const auto &frame = Graphics3D_GetFrame(gGfx);
	if (UpdateUIInfo) {
		UI::FrameRate = FrameRate;
		UI::FrameTime = dTime * 1e3;
		shadow->GetStatistics(false, frame.m_FrameIndex, UI::ShadowPassTime);
		cullPass->GetComputeShaderStatistics(false, frame.m_FrameIndex, UI::FrustrumCullingTime, UI::FrustrumInvocations);
		geoPass->GetStatistics(false, frame.m_FrameIndex, UI::GeometryPassTime, UI::VertexInvocations, UI::FragmentInvocations);
		bloom->GetStatistics(false, frame.m_FrameIndex, UI::BloomPassTime);
		debugPass->GetStatistics(false, frame.m_FrameIndex, UI::DebugPassTime);
	}
	
	glm::vec3 pos = glm::vec3(UI::L_x, UI::L_y, UI::L_z);
	glm::mat4 proj = glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->GetWidth(), (float)gWindow->GetHeight(), 0.1f, 1000.0f);
	debugPass->SetProjectionView(proj, gCamera.GetViewMatrix());

	skybox->SetLOD(UI::CubemapLOD);

	shadow->SetShadowLightPosition(pos);
	geoPass->SetLightDirection(glm::normalize(pos));
	geoPass->SetLightSpace(shadow->GetLightSpace());

	debugPass->NewFrame();
	srand(0x42);
	glm::vec3 color = glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
	color *= 5.0f;
	debugPass->DrawCube(pos * glm::vec3(1.0, -1.0, 1.0), {1.0, 1.0, 1.0}, color);

	return true;
}

void Application::Render(double dTimeFromStart, double dTime, bool UpdateUITime)
{
	PROFILE_FUNCTION();
	VkDevice device = gContext->defaultDevice;
	auto& frame = Graphics3D_GetFrame(gGfx);
	uint32_t FrameIndex = frame.m_FrameIndex;
	
	auto NextFrameStart = std::chrono::high_resolution_clock::now();
	if (gSwapchain.PrepareNextFrame(UINT64_MAX, frame.m_RenderSemaphore, nullptr, nullptr) != VK_SUCCESS) {
		return;
	}
	auto NextFrameEnd = std::chrono::high_resolution_clock::now();
	if(UpdateUITime)
		UI::NextFrameTime = double((NextFrameEnd - NextFrameStart).count()) * 1e-6;

	vkWaitForFences(device, 1, &frame.m_RenderFence, true, UINT64_MAX);
	vkResetFences(device, 1, &frame.m_RenderFence);
	
	VkCommandBuffer cullPassShadowCmd = nullptr;
	VkCommandBuffer cullPassCmd = cullPass->Prepare(frame.m_FrameIndex, dTime, dTimeFromStart);
	VkCommandBuffer shadowPassCmd = shadow->Prepare(frame.m_FrameIndex, dTime, dTimeFromStart);

	vk::Gfx_SubmitCmdBuffers(gContext->defaultQueue, { cullPassCmd, shadowPassCmd }, { frame.m_RenderSemaphore }, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { shadowPassSemaphore[FrameIndex] }, nullptr);

	VkCommandBuffer geoPassCmd = geoPass->Prepare(frame.m_FrameIndex, dTime, dTimeFromStart);
	VkCommandBuffer skyboxCmd = skybox->Prepare(frame.m_FrameIndex, dTime, dTimeFromStart);
	VkCommandBuffer debugPassCmd = debugPass->Prepare(frame.m_FrameIndex, dTime, dTimeFromStart);

	vk::Gfx_SubmitCmdBuffers(gContext->defaultQueue, 
		{ geoPassCmd, skyboxCmd, debugPassCmd }, { shadowPassSemaphore[FrameIndex] }, { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT }, { bloomPassSemaphore[FrameIndex] }, nullptr);

	VkCommandBuffer bloomCmd = bloom->Prepare(FrameIndex, dTime, dTimeFromStart);
	
	vk::Gfx_SubmitCmdBuffers(gContext->defaultQueue, { bloomCmd }, { bloomPassSemaphore[FrameIndex] }, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { frame.m_PresentSemaphore }, frame.m_RenderFence);
	
	if (s_EnableImGui)
		UI::RenderUI();
	if (UI::CurrentOutputBuffer == 0) {
		gSwapchain.Present(gFBO0.m_color_attachments[0].GetImage(FrameIndex), gFBO0.m_color_attachments[0].GetView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, false);
	}
	else if(UI::CurrentOutputBuffer == 1) {
		gSwapchain.Present(gFBO0.m_depth_attachment.value().GetImage(FrameIndex), gFBO0.m_depth_attachment.value().GetView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, true);
	}
	else if (UI::CurrentOutputBuffer == 2) {
		gSwapchain.Present(shadow->GetDepthImage(FrameIndex), shadow->GetDepthView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, &frame.m_PresentSemaphore, true);
	}
	else if (UI::CurrentOutputBuffer == 3) {
		gSwapchain.Present(bloom->GetDownsampleTexture()->m_vk_images_per_frame[FrameIndex], bloom->GetDownsampleTexture()->mMipmapViews.mMipmapViewsPerFrame[FrameIndex][UI::BloomDownsampleMip], VK_IMAGE_LAYOUT_GENERAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, false);
	}
	Graphics3D_NextFrame(gGfx);
}

void Application::Destroy()
{
	PROFILE_FUNCTION();
	Graphics3D_WaitGPUIdle(gGfx);
	delete cullPass;
	delete geoPass;
	delete skybox;
	delete shadow;
	delete debugPass;
	delete bloom;
	for (int i = 0; i < gFrameOverlapCount; i++) {
		vkDestroySemaphore(gContext->defaultDevice, shadowPassSemaphore[i], nullptr);
		vkDestroySemaphore(gContext->defaultDevice, bloomPassSemaphore[i], nullptr);
	}
	Buffer2_Destroy(gECS->GetEntity(EntityGeometryID::ENTITY_GEOMETRY_CUBE)->mInstanceBuffer);
	Buffer2_Destroy(gECS->GetEntity(EntityGeometryID::ENTITY_GEOMETRY_CUBE)->mCulledInstanceBuffer);
	gFBO0.DestroyAllBoundAttachments();
	Buffer2_Destroy(gVerticesSSBO);
	Buffer2_Destroy(gIndicesBuffer);
	Physx_Destroy();
	delete gECS;
	Graphics3D_Destroy(gGfx);
}
