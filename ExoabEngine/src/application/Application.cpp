#include "Application.hpp"
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
#include "perlin_noise.hpp"
#include "../audio/Audio.hpp"
#include "../assets/geometry.cfg"

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
	
	EntityController* gECS;
	static FrustumCullPass* cullPass;
	static GeometryPass* geoPass;
	static SkyboxPass* skybox;
	static ShadowPass* shadow;
	static DebugPass* debugPass;
	static BloomPass* bloom;
	static VkSemaphore shadowPassSemaphore[gFrameOverlapCount];
	static VkSemaphore bloomPassSemaphore[gFrameOverlapCount];
	static Terrain* t0 = nullptr;
	static uint32_t instanceCount = 500;
	static ITexture2 noiseMapTexture;
	ShaderTypes::InstanceData* instance = new ShaderTypes::InstanceData[instanceCount];

}

bool Application::Initalize(ConfigurationSettings* configuration, bool RenderDoc)
{
	PROFILE_FUNCTION();
	Audio* audio = new Audio();
	AudioBuffer* buffer = new AudioBuffer(audio, 44100, 15.0);
	//float* fbuf = buffer->GetBuffer();
	//float step = 1.0 / (44100 * 2);
	//for (int i = 0; i < (44100 * 2 * 15); i++) {
	//	fbuf[i] = sinf(step * i * 50.0 * 2.0 * 3.14);
	//}
	buffer->Play();
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
	uint32_t instanceSize = instanceCount * sizeof(ShaderTypes::InstanceData);
	for (unsigned int i = 0; i < instanceCount; i++) {
		float x = (rand() % 25) * 5;
		float y = (rand() % 25) * 5;
		float z = (rand() % 25) * 5;
		glm::vec3 offset = glm::vec3(x, y, z + 2);
		offset = offset - (offset / glm::vec3(2.0));
		instance[i].mModel = glm::translate(glm::scale(glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0)), glm::vec3(1)), offset);
		instance[i].mNormalModel = ShaderTypes::CalculateNormalModel(instance[i].mModel);
		instance[i].mTextureIndex = 0;
	}

	IEntity cube = gECS->GetEntity(ENTITY_GEOMETRY_CUBE);
	cube->m_geometryID = ENTITY_GEOMETRY_CUBE;
	cube->mInstanceCount = instanceCount;
	cube->mInstanceBuffer = Buffer2_CreatePreInitalized(BUFFER_TYPE_STORAGE, &instance[0], instanceSize, BufferMemoryType::CPU_TO_GPU, true, false);
	cube->mCulledInstanceBuffer = Buffer2_Create(BUFFER_TYPE_STORAGE, instanceSize, BufferMemoryType::GPU_ONLY, true, false);
	
	return true;
}

static void CreateTerrain() {
	int w = 256;
	int h = 256;
	int hw = 512;
	int hh = 512;
	if(!Application::t0)
		Application::t0 = new Terrain(w, h, 4, 4);
	std::vector<float> perlinBuffer(hw * hh);
	Utils::perlin(hw, hh, std::chrono::high_resolution_clock::now().time_since_epoch().count(), UI::Frequency, UI::Octave, perlinBuffer.data());
	Application::t0->ApplyHeightmap(hw, hh, 0.0, 10.0f, perlinBuffer.data());
	Application::t0->SetTransform(glm::scale(glm::mat4(1.0), glm::vec3(2.0)));

	if (!Application::noiseMapTexture) {
		Texture2DSpecification specifcation;
		specifcation.m_Width = hw;
		specifcation.m_Height = hh;
		specifcation.m_Format = VK_FORMAT_R32_SFLOAT;
		specifcation.mTextureSwizzling.r = VK_COMPONENT_SWIZZLE_R;
		specifcation.mTextureSwizzling.g = VK_COMPONENT_SWIZZLE_R;
		specifcation.mTextureSwizzling.b = VK_COMPONENT_SWIZZLE_R;
		Application::noiseMapTexture = Texture2_Create(gContext, specifcation);
	}
	Texture2_UploadPixels(Application::noiseMapTexture, perlinBuffer.data(), perlinBuffer.size() * sizeof(float));
}

bool Application::CreateResources()
{
	PROFILE_FUNCTION();
	
	UI::Frequency = 8.0;
	UI::Octave = 2;
	CreateTerrain();
	UI::UpdateNoiseMap(noiseMapTexture, vk::Gfx_CreateSampler(gContext));

	shadow = new ShadowPass(gVerticesSSBO, gIndicesBuffer, t0, gECS, &gCamera, 2048);
	cullPass = new FrustumCullPass(gECS, &gLockedCamera);
	geoPass = new GeometryPass(gVerticesSSBO, gIndicesBuffer, t0, 1920, 1080, cullPass, &gCamera, gECS, shadow->GetDepthAttachment());
	skybox = new SkyboxPass("assets/textures/cubemap4.png", geoPass, &gCamera, true);
	debugPass = new DebugPass(geoPass->GetFramebuffer());
	bloom = new BloomPass(geoPass->GetFramebuffer(), 0, 1.0);
	UI::CubemapLODMax = skybox->GetMaxLOD();

	for (int i = 0; i < gFrameOverlapCount; i++) {
		shadowPassSemaphore[i] = vk::Gfx_CreateSemaphore(gContext, false);
		bloomPassSemaphore[i] = vk::Gfx_CreateSemaphore(gContext, false);
	}

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
	UI::CameraPosition = position;
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

	if (UI::RegenerateNoiseMap) {
		UI::RegenerateNoiseMap = false;
		CreateTerrain();
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
	
	glm::vec3 pos = UI::LightPosition;
	glm::mat4 proj = glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->GetWidth(), (float)gWindow->GetHeight(), 0.1f, 1000.0f);
	debugPass->SetProjectionView(proj, gCamera.GetViewMatrix());

	skybox->SetLOD(UI::CubemapLOD);

	shadow->SetShadowLightPosition(pos);
	geoPass->SetLightDirection(-1.0f * glm::normalize(pos));
	geoPass->SetLightSpace(shadow->GetLightSpace());

	debugPass->NewFrame();
	srand(0x42);
	glm::vec3 color = glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
	color *= 5.0f;
	debugPass->DrawCube(pos * glm::vec3(1.0, -1.0, 1.0), {1.0, 1.0, 1.0}, color, false);

	for (int i = 0; i < t0->GetSubmeshCount(); i++) {
		auto& submesh = t0->GetSubmesh(i);
		debugPass->DrawCube(submesh.mBox.mBoxMin + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.25), glm::vec3(52, 140, 235) / glm::vec3(255), false);
		debugPass->DrawCube(submesh.mBox.mBoxMax, glm::vec3(0.25), glm::vec3(227, 18, 60) / glm::vec3(255), false);
	}

	for (int i = 0; i < instanceCount; i++) {
		glm::vec3 centerPos = instance[i].mModel * glm::vec4(gGeomtry[0].m_bounding_sphere_center, 1.0);
		glm::vec3 radPos = glm::vec3(instance[i].mModel * glm::vec4(gGeomtry[0].m_bounding_sphere_center, 1.0)) + glm::vec3(gGeomtry[0].m_bounding_sphere_radius, 0, 0);
		debugPass->DrawCube(centerPos, glm::vec3(0.25), glm::vec3(196, 26, 201) / glm::vec3(255), false);
		debugPass->DrawCube(radPos, glm::vec3(0.25), glm::vec3(26, 173, 63) / glm::vec3(255), false);
	}

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
		gSwapchain.Present(geoPass->GetFramebuffer().m_color_attachments[0].GetImage(FrameIndex), geoPass->GetFramebuffer().m_color_attachments[0].GetView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, false);
	}
	else if(UI::CurrentOutputBuffer == 1) {
		gSwapchain.Present(geoPass->GetFramebuffer().m_depth_attachment.value().GetImage(FrameIndex), geoPass->GetFramebuffer().m_depth_attachment.value().GetView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, true);
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
	Texture2_Destroy(noiseMapTexture);
	delete t0;
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
	Buffer2_Destroy(gECS->GetEntity(ENTITY_GEOMETRY_CUBE)->mInstanceBuffer);
	Buffer2_Destroy(gECS->GetEntity(ENTITY_GEOMETRY_CUBE)->mCulledInstanceBuffer);
	Buffer2_Destroy(gVerticesSSBO);
	Buffer2_Destroy(gIndicesBuffer);
	delete gECS;
	Graphics3D_Destroy(gGfx);
}
