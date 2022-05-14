#include "Application.hpp"
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#ifdef _DEBUG
static constexpr bool s_DebugMode = true;
static constexpr const char* s_ShaderCache = "assets/shaders/spirv_cache/";
#else
static constexpr bool s_DebugMode = false;
static constexpr const char* s_ShaderCache = "assets/shaders/spirv_cacheoptimized/";
#endif
static constexpr bool s_EnableImGui = true;

namespace Global {
	bool Quit = false;
	vk::VkContext Context = nullptr;
	PlatformWindow* Window = nullptr;
	glm::mat4 Projection;
	std::vector<Mesh::Geometry> Geomtry;
	VkSampler DefaultSampler;
	float zNear = 0.5;
	float zFar = 10000.0;
}

namespace Application
{
	bool Application::OnInitalize()
	{
		PROFILE_FUNCTION();
		loginfo(__FUNCSIG__);
#if 0
		Audio* audio = new Audio();
		AudioBuffer* buffer = new AudioBuffer(audio, 44100, 15.0);
		float* fbuf = buffer->GetBuffer();
		float step = 1.0 / (44100 * 2);
		for (int i = 0; i < (44100 * 2 * 15); i++) {
			fbuf[i] = sinf(step * i * 50.0 * 2.0 * 3.14);
		}
		buffer->Play();
#endif
		log_configure(true, true);
		if (!Graphics3D_CheckVulkanSupport())
		{
			log_error("Vulkan is not supported, try updating your graphics driver.");
			return false;
		}

		mGfx = Graphics3D_Create(&Global::Configuration, Global::zNear, Global::zFar, Global::Configuration.WindowTitle.c_str(), s_DebugMode, s_EnableImGui, Global::RenderDOC);
		Global::Context = (mGfx->m_context);
		Loader_LoadVulkan();
		Loader_LoadInstance(Global::Context->instance);
		Loader_LoadDevice(Global::Context->defaultDevice);

		struct ImGUI_LoadFunctions_UserData
		{
			VkInstance instance;
			VkDevice device;
		};
		ImGUI_LoadFunctions_UserData ImGui_Load_UserData;
		ImGui_Load_UserData.instance = Global::Context->instance;
		ImGui_Load_UserData.device = Global::Context->defaultDevice;
		ImGui_ImplVulkan_LoadFunctions(
			[](const char* function_name, void* user_data) throw()->PFN_vkVoidFunction
			{
				ImGUI_LoadFunctions_UserData* pUserData = (ImGUI_LoadFunctions_UserData*)user_data;
				PFN_vkVoidFunction proc = (PFN_vkVoidFunction)vkGetDeviceProcAddr(pUserData->device, function_name);
				if (!proc)
					return vkGetInstanceProcAddr(pUserData->instance, function_name);
				else
					return proc;
			},
			&ImGui_Load_UserData);

		ImGui::SetCurrentContext((ImGuiContext*)Graphics3D_GetCurrentImGuiContext());
		mSwapchain = mGfx->m_vswapchain;
		Global::Window = mGfx->m_window;
		Global::DefaultSampler = vk::Gfx_CreateSampler(Global::Context);
		Global::Projection = glm::perspectiveFovLH<float>(90.0, Global::Window->GetWidth(), Global::Window->GetHeight(), Global::zNear, Global::zFar);
		Shader::ConfigureShaderCache(s_ShaderCache);

		{
			int w, h, c;
			unsigned char* p = (uint8_t*)stbi_load("assets/textures/idleCursor.png", &w, &h, &c, 4);
			GLFWimage image;
			image.width = w;
			image.height = h;
			image.pixels = p;
			auto cursor = glfwCreateCursor(&image, 0, 0);
			glfwSetCursor(Global::Window->GetWindow(), cursor);
			glfwDestroyCursor(cursor);
			glfwPollEvents();
			free(p);
		}

		if (s_EnableImGui)
			UI::Initalize(Global::Context, mGfx);

		Mesh::GeometryConfiguration config;
		config.Load("assets/geometry.cfg");
		bool MeshLoadStatus = Mesh::LoadVerticesIndicesSSBOs(Global::Context, config, Global::Geomtry, &mVerticesSSBO, &mIndicesBuffer);
		if (!MeshLoadStatus)
			return false;

		mECS = new ecs::EntityController(Global::Geomtry);

		mEngine = new Ph::PhysicsEngine(50, glm::vec3(0.1, -9.8, 0.0));
		//srand(140);
#if 1
		ecs::IEntityGeometry cube = mECS->GetEntity(ENTITY_GEOMETRY_CUBE);
		ECS_ConfigureEntityGeometry(cube, mInstanceCount);
		uint32_t instanceSize = mInstanceCount * sizeof(ShaderTypes::InstanceData);
		int m0 = mEngine->CreateMaterial(3.1, 2.0, 0.0);
		for (unsigned int i = 0; i < mInstanceCount; i++) {
			float x = (rand() % 25) * 5;
			float y = (rand() % 25) * 5;
			float z = (rand() % 25) * 5;
			glm::vec3 offset = glm::vec3(x, y, z + 2);
			offset = offset - (offset / glm::vec3(2.0));
			ShaderTypes::InstanceData sData;
			sData.mModel = glm::translate(glm::mat4(1.0), offset);
			sData.mNormalModel = ShaderTypes::CalculateNormalModel(sData.mModel);
			sData.mTextureIndex = 0;
			sData.mSpecularStrength = 1.0;
			sData.nSelectedData = 0;
			vEnts.push_back(mEngine->CreateRigidEntity(cube, sData, m0, true));
		}
#endif
		int m1 = mEngine->CreateMaterial(0.5, 0.6, 0.5);
		/* Prevent Objects from getting out of the world */
		float worldSize = 500.0f;
		mEngine->AddPlane(glm::vec4(+0.0, +1.0, +0.0, +1.0), m1);
		mEngine->AddPlane(glm::vec4(+0.0, -1.0, +0.0, worldSize), m1);
		mEngine->AddPlane(glm::vec4(+1.0, +0.0, +0.0, worldSize), m1);
		mEngine->AddPlane(glm::vec4(-1.0, +0.0, +0.0, worldSize), m1);
		mEngine->AddPlane(glm::vec4(+0.0, +0.0, +1.0, worldSize), m1);
		mEngine->AddPlane(glm::vec4(+0.0, +0.0, -1.0, worldSize), m1);
		

		PROFILE_FUNCTION();
		UI::Frequency[0] = 2;
		UI::Octave[0] = 2;
		UI::Frequency[1] = 2;
		UI::Octave[1] = 4;
		UI::Frequency[2] = 8.0;
		UI::Octave[2] = 4;
		UI::Contribution[0] = 1.0;
		UI::Contribution[1] = 0.5;
		UI::Contribution[2] = 0.2;
		CreateTerrain();
		mImGuiSampler = vk::Gfx_CreateSampler(Global::Context);
		UI::UpdateNoiseMap(mNoiseMapTexture, mNoiseMapTexture1, mNoiseMapTexture2, mImGuiSampler);
		
		UI::LightPosition = glm::vec3(0.0f, 25.0f, 2.5f);

		mCamera = new CameraControl(Global::Window);

		mShadow = new ShadowPass(mVerticesSSBO, mIndicesBuffer, mT0, mECS, &mCamera->mCamera, 1024);
		mCullPass = new FrustumCullPass(mECS, &mCamera->mLockedCamera);
		mGeoPass = new GeometryPass(1, &mLight, mVerticesSSBO, mIndicesBuffer, mT0, 1920, 1080, mCullPass, &mCamera->mCamera, &mCamera->mLockedCamera, mECS, mShadow->GetDepthAttachment());
		mSkybox = new SkyboxPass("assets/textures/cubemap4.png", mGeoPass, &mCamera->mCamera, true);
		mDebugPass = new DebugPass(mGeoPass->GetFramebuffer());
		mBloom = new BloomPass(mGeoPass->GetFramebuffer(), 0, 1.0);
		mMinimap = mGeoPass->CreateMinimap();
		mUI = new GameUI(mGeoPass->GetFramebuffer(), 0, mMinimap);
		UI::CubemapLODMax = mSkybox->GetMaxLOD();
		mCamera->SetGameUI(mUI);

		for (int i = 0; i < gFrameOverlapCount; i++) {
			mShadowPassSemaphore[i] = vk::Gfx_CreateSemaphore(Global::Context, false);
			mBloomPassSemaphore[i] = vk::Gfx_CreateSemaphore(Global::Context, false);
		}

		return true;

	}

	void Application::OnFrame() {
		PROFILE_FUNCTION();
		if (!Graphics3D_PollEvents(mGfx))
			Global::Quit = true;

		Global::Projection = glm::perspectiveFovLH<float>(90.0, Global::Window->GetWidth(), Global::Window->GetHeight(), 0.5f, 10000.0f);
		mCamera->LoadViewMatrix();
		mEngine->Start();
		mEngine->End();

		for (auto ent : vEnts) {
			ent->Update();
			/*
				Determine selected units
			*/

		}
		
		mECS->PrepareDataForFrame();

		if (UI::StateChanged)
		{
			Graphics3D_WaitGPUIdle(mGfx);
			UI::StateChanged = false;
			if (UI::ClearShaderCache) {
				UI::ClearShaderCache = false;
				logalert("Deleting Shader Cache.");
				std::filesystem::remove_all(s_ShaderCache);
			}
			if (UI::ReloadShaders) {
				UI::ReloadShaders = false;
				logalert("Reloading All Scene Shaders.");
				mCullPass->ReloadShaders();
				mGeoPass->ReloadShaders();
				mSkybox->ReloadShaders();
				mShadow->ReloadShaders();
				mDebugPass->ReloadShaders();
				mBloom->ReloadShaders();
				mUI->ReloadShaders();
			}
			mGeoPass->SetWireframeMode(UI::ShowWireframe);
			if (UI::SaveTerrain) {
				int choosenFilter = 0;
				std::vector<std::string> assimpIDs;
				std::vector<std::pair<std::string, std::string>> filters = Terrain::GetFilters(assimpIDs);
				std::string path = Utils::SaveAsDialog(Utils::CreateDialogFilter(filters), choosenFilter);
				if (path.size() > 0) {
					if (!Utils::StrContains(path, "."))
						path += filters[choosenFilter].second.data() + 1;
					mT0->Export(assimpIDs[choosenFilter].data(), path);
					mT0->Save(path + ".zip");
				}
			}
		}

		if (UI::RegenerateNoiseMap) {
			UI::RegenerateNoiseMap = false;
			CreateTerrain();
		}

		if (UI::LockFrustrum) {
			mCamera->LockCamera();
		}
		else {
			mCamera->UnlockCamera();
		}

		const auto& frame = Graphics3D_GetFrame(mGfx);
		if (Global::UpdateUIInfo) {
			UI::FrameRate = Global::FrameRate;
			UI::FrameTime = Global::Time * 1e3;
			const bool wait = false;
			mShadow->GetStatistics(wait, frame.m_FrameIndex, UI::ShadowPassTime);
			mCullPass->GetComputeShaderStatistics(wait, frame.m_FrameIndex, UI::FrustrumCullingTime, UI::FrustrumInvocations);
			mGeoPass->GetStatistics(wait, frame.m_FrameIndex, UI::GeometryPassTime, UI::VertexInvocations, UI::FragmentInvocations);
			mBloom->GetStatistics(wait, frame.m_FrameIndex, UI::BloomPassTime);
			mDebugPass->GetStatistics(wait, frame.m_FrameIndex, UI::DebugPassTime);
		}
		
		//glm::mat4 transform = glm::translate(glm::mat4(1.0), glm::vec3(t0->, 0.0, 0.0));
		//t0->SetTransform(transform);

		mDebugPass->NewFrame();
		if (UI::LockFrustrum) {
			float width = Global::Window->GetWidth();
			float height = Global::Window->GetHeight();
			glm::mat4 proj = Global::Projection;
			glm::mat4 view = mCamera->ReadLockedViewMatrix();
		
			//(-1, -1, -1) (1, -1, -1) (1, 1, -1) (-1, 1, -1) // near plane
			//(-1, -1, 1) (1, -1, 1) (1, 1, 1) (-1, 1, 1) // far plane
		
			glm::vec3 nearPlane[4];
			nearPlane[0] = glm::vec3(-1.0f);
			nearPlane[1] = glm::vec3(1.0f, -1.0f, -1.0f);
			nearPlane[2] = glm::vec3(1.0f, 1.0f, -1.0f);
			nearPlane[3] = glm::vec3(-1.0f, 1.0f, -1.0f);
		
			glm::vec3 farPlane[4];
			farPlane[0] = glm::vec3(-1.0f, -1.0f, 1.0f);
			farPlane[1] = glm::vec3(1.0f, -1.0f, 1.0f);
			farPlane[2] = glm::vec3(1.0f);
			farPlane[3] = glm::vec3(-1.0f, 1.0f, 1.0f);
		
			glm::mat4 invProj = glm::inverse(proj);
			glm::mat4 invView = glm::inverse(view);
			auto ToWorldPosition = [&invProj, &invView](glm::vec3& ndc) throw() -> void {
				glm::vec4 pth = invView * invProj * glm::vec4(ndc, 1.0f);
				ndc = glm::vec3(pth) / pth.w;
			};
		
			ToWorldPosition(nearPlane[0]);
			ToWorldPosition(nearPlane[1]);
			ToWorldPosition(nearPlane[2]);
			ToWorldPosition(nearPlane[3]);
		
			ToWorldPosition(farPlane[0]);
			ToWorldPosition(farPlane[1]);
			ToWorldPosition(farPlane[2]);
			ToWorldPosition(farPlane[3]);
		
			mDebugPass->DrawLine(nearPlane[0], nearPlane[1], glm::vec3(1.0, 0.0, 0.0), 6);
			mDebugPass->DrawLine(nearPlane[2], nearPlane[1], glm::vec3(1.0, 0.0, 0.0), 6);
			mDebugPass->DrawLine(nearPlane[3], nearPlane[2], glm::vec3(1.0, 0.0, 0.0), 6);
			mDebugPass->DrawLine(nearPlane[3], nearPlane[0], glm::vec3(1.0, 0.0, 0.0), 6);
			
			mDebugPass->DrawLine(farPlane[0], farPlane[1], glm::vec3(1.0), 6);
			mDebugPass->DrawLine(farPlane[1], farPlane[2], glm::vec3(1.0), 6);
			mDebugPass->DrawLine(farPlane[2], farPlane[3], glm::vec3(1.0), 6);
			mDebugPass->DrawLine(farPlane[3], farPlane[0], glm::vec3(1.0), 6);
			
			mDebugPass->DrawLine(nearPlane[0], farPlane[0], glm::vec3(1.0), 4);
			mDebugPass->DrawLine(nearPlane[1], farPlane[1], glm::vec3(1.0), 4);
			mDebugPass->DrawLine(nearPlane[2], farPlane[2], glm::vec3(1.0), 4);
			mDebugPass->DrawLine(nearPlane[3], farPlane[3], glm::vec3(1.0), 4);
		}
		
		glm::vec3 pos = UI::LightPosition;
		mLight.u_AmbientStrength = 0.1;
		mLight.u_Color = glm::vec3(1.0);
		mLight.u_Position = mCamera->mCamera.GetPosition();
		mLight.u_Range = 100;
		mDebugPass->SetProjectionView(Global::Projection, mCamera->ReadViewMatrix());

		mSkybox->SetLOD(UI::CubemapLOD);
		
		mShadow->SetShadowLightPosition(pos);
		mGeoPass->SetLightDirection(-1.0f * glm::normalize(pos));
		mGeoPass->SetLightSpace(mShadow->GetLightSpace());

		mUI->SetSelectBox(mCamera->sMouseSelect.bSelect, mCamera->sMouseSelect.iA, mCamera->sMouseSelect.iB, Global::Window->GetWidth(), Global::Window->GetHeight());

		srand(0x42);
		glm::vec3 color = glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
		color *= 5.0f;
		mDebugPass->DrawCube(pos * glm::vec3(1.0, -1.0, 1.0), { 1.0, 1.0, 1.0 }, color, false, 1);

		for (uint32_t i = 0; i < mT0->GetSubmeshCount(); i++) {
			auto& submesh = mT0->GetSubmesh(i);
			glm::vec3 centerPos = mT0->GetTransform() * glm::vec4(submesh.mSphere.mCenter, 1.0);
			glm::vec3 radPos = glm::vec3(mT0->GetTransform() * glm::vec4(centerPos, 1.0)) + glm::vec3(submesh.mSphere.mRadius, 0, 0);
			mDebugPass->DrawCube(centerPos, glm::vec3(0.25), glm::vec3(52, 140, 235) / glm::vec3(255), false, mInstanceCount);
			mDebugPass->DrawCube(radPos, glm::vec3(0.25), glm::vec3(227, 18, 60) / glm::vec3(255), false, mInstanceCount);
		}

#if 0
		for (uint32_t i = 0; i < mInstanceCount; i++) {
			glm::vec3 centerPos = mInstances[i].mModel * glm::vec4(Global::Geomtry[0].m_bounding_sphere_center, 1.0);
			glm::vec3 radPos = glm::vec3(mInstances[i].mModel * glm::vec4(Global::Geomtry[0].m_bounding_sphere_center, 1.0)) + glm::vec3(Global::Geomtry[0].m_bounding_sphere_radius, 0, 0);
			mDebugPass->DrawCube(centerPos, glm::vec3(0.25), glm::vec3(196, 26, 201) / glm::vec3(255), false, mInstanceCount);
			mDebugPass->DrawCube(radPos, glm::vec3(0.25), glm::vec3(26, 173, 63) / glm::vec3(255), false, mInstanceCount);
		}
#endif

		VkDevice device = Global::Context->defaultDevice;
		uint32_t FrameIndex = frame.m_FrameIndex;

		auto NextFrameStart = std::chrono::high_resolution_clock::now();
		if (mSwapchain.PrepareNextFrame(UINT64_MAX, frame.m_RenderSemaphore, nullptr, nullptr) != VK_SUCCESS) {
			return;
		}
		auto NextFrameEnd = std::chrono::high_resolution_clock::now();
		Global::Context->mFrameIndex = FrameIndex;
		if (Global::UpdateUIInfo)
			UI::NextFrameTime = double((NextFrameEnd - NextFrameStart).count()) * 1e-6;
		
		vkWaitForFences(device, 1, &frame.m_RenderFence, true, UINT64_MAX);
		vkResetFences(device, 1, &frame.m_RenderFence);
		
		// Culling and Shadow Pass
		VkCommandBuffer cullPassShadowCmd = nullptr;
		VkCommandBuffer cullPassCmd = mCullPass->Prepare(frame.m_FrameIndex, Global::Time, Global::TimeFromStart);
		VkCommandBuffer shadowPassCmd = mShadow->Prepare(frame.m_FrameIndex, Global::Time, Global::TimeFromStart);
		
		vk::Gfx_SubmitCmdBuffers(Global::Context->defaultQueue, { cullPassCmd, shadowPassCmd }, { frame.m_RenderSemaphore }, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { mShadowPassSemaphore[FrameIndex] }, nullptr);
		
		// Geometry Pass
		VkCommandBuffer geoPassCmd = mGeoPass->Prepare(frame.m_FrameIndex, Global::Time, Global::TimeFromStart);
		VkCommandBuffer skyboxCmd = mSkybox->Prepare(frame.m_FrameIndex, Global::Time, Global::TimeFromStart);
		VkCommandBuffer debugPassCmd = mDebugPass->Prepare(frame.m_FrameIndex, Global::Time, Global::TimeFromStart);

		vk::Gfx_SubmitCmdBuffers(Global::Context->defaultQueue,
			{ geoPassCmd, skyboxCmd, debugPassCmd }, { mShadowPassSemaphore[FrameIndex] }, { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT }, { mBloomPassSemaphore[FrameIndex] }, nullptr);
		
		// Postprocess Effects
		VkCommandBuffer gameUICmd = mUI->Prepare(frame.m_FrameIndex, Global::Time, Global::TimeFromStart);
		VkCommandBuffer bloomCmd = mBloom->Prepare(FrameIndex, Global::Time, Global::TimeFromStart);
		
		vk::Gfx_SubmitCmdBuffers(Global::Context->defaultQueue, { bloomCmd, gameUICmd }, { mBloomPassSemaphore[FrameIndex] }, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { frame.m_PresentSemaphore }, frame.m_RenderFence);
		
		if (s_EnableImGui)
			UI::RenderUI();
		mSwapchain.SetExposure(UI::Exposure);
		if (UI::CurrentOutputBuffer == 0) {
			mSwapchain.Present(mGeoPass->GetFramebuffer().m_color_attachments[0].GetImage(FrameIndex), mGeoPass->GetFramebuffer().m_color_attachments[0].GetView(FrameIndex), VK_IMAGE_LAYOUT_GENERAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, false);
		}
		else if (UI::CurrentOutputBuffer == 1) {
			mSwapchain.Present(mGeoPass->GetFramebuffer().m_depth_attachment.value().GetImage(FrameIndex), mGeoPass->GetFramebuffer().m_depth_attachment.value().GetView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, true);
		}
		else if (UI::CurrentOutputBuffer == 2) {
			mSwapchain.Present(mShadow->GetDepthImage(FrameIndex), mShadow->GetDepthView(FrameIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, true);
		}
		else if (UI::CurrentOutputBuffer == 3) {
			mSwapchain.Present(mBloom->GetDownsampleTexture()->m_vk_images_per_frame[FrameIndex], mBloom->GetDownsampleTexture()->mMipmapViews.mMipmapViewsPerFrame[FrameIndex][UI::BloomDownsampleMip], VK_IMAGE_LAYOUT_GENERAL, 1, (VkSemaphore*)&frame.m_PresentSemaphore, false);
		}
		Graphics3D_NextFrame(mGfx);

	}

	void Application::OnDestroy() {
		PROFILE_FUNCTION();
		Graphics3D_WaitGPUIdle(mGfx);
		delete mCamera;
		for (auto e : vEnts)
			mEngine->DestroyRigidEntity(e);
		delete mEngine;

		vkDestroySampler(Global::Context->defaultDevice, Global::DefaultSampler, nullptr);
		Texture2_Destroy(mMinimap);
		Texture2_Destroy(mNoiseMapTexture);
		Texture2_Destroy(mNoiseMapTexture1);
		Texture2_Destroy(mNoiseMapTexture2);
		delete mT0;
		delete mCullPass;
		delete mGeoPass;
		delete mSkybox;
		delete mShadow;
		delete mDebugPass;
		delete mBloom;
		delete mUI;
		for (int i = 0; i < gFrameOverlapCount; i++) {
			vkDestroySemaphore(Global::Context->defaultDevice, mShadowPassSemaphore[i], nullptr);
			vkDestroySemaphore(Global::Context->defaultDevice, mBloomPassSemaphore[i], nullptr);
		}
		vkDestroySampler(Global::Context->defaultDevice, mImGuiSampler, nullptr);
		Buffer2_Destroy(mVerticesSSBO);
		Buffer2_Destroy(mIndicesBuffer);
		delete mECS;
		Graphics3D_Destroy(mGfx);
	}

	void Application::CreateTerrain() {
		int w = 256;
		int h = 256;
		int hw = 256;
		int hh = 256;
		std::vector<float> perlinBuffer(hw * hh);
		std::vector<float> perlinBuffer1(hw * hh);
		std::vector<float> perlinBuffer2(hw * hh);
		if (UI::ActiveNoiseMap == 0 || !(mT0)) {
			Utils::perlin(hw, hh, std::chrono::high_resolution_clock::now().time_since_epoch().count(), UI::Frequency[0], UI::Octave[0], perlinBuffer.data());
		}
		if (UI::ActiveNoiseMap == 1 || !(mT0)) {
			Utils::perlin(hw, hh, std::chrono::high_resolution_clock::now().time_since_epoch().count(), UI::Frequency[1], UI::Octave[1], perlinBuffer1.data());
		}
		if (UI::ActiveNoiseMap == 2 || !(mT0)) {
			Utils::perlin(hw, hh, std::chrono::high_resolution_clock::now().time_since_epoch().count(), UI::Frequency[2], UI::Octave[2], perlinBuffer2.data());
		}
		if (!mNoiseMapTexture) {
			Texture2DSpecification specifcation;
			specifcation.m_Width = hw;
			specifcation.m_Height = hh;
			specifcation.m_Format = VK_FORMAT_R32_SFLOAT;
			specifcation.mTextureSwizzling.r = VK_COMPONENT_SWIZZLE_R;
			specifcation.mTextureSwizzling.g = VK_COMPONENT_SWIZZLE_R;
			specifcation.mTextureSwizzling.b = VK_COMPONENT_SWIZZLE_R;
			mNoiseMapTexture = Texture2_Create (Global::Context, specifcation);
			mNoiseMapTexture1 = Texture2_Create(Global::Context, specifcation);
			mNoiseMapTexture2 = Texture2_Create(Global::Context, specifcation);
		}
		if (UI::ActiveNoiseMap == 0 || !(mT0)) {
			Texture2_UploadPixels(mNoiseMapTexture, perlinBuffer.data(), perlinBuffer.size() * sizeof(float));
		}
		if (UI::ActiveNoiseMap == 1 || !(mT0)) {
			Texture2_UploadPixels(mNoiseMapTexture1, perlinBuffer1.data(), perlinBuffer1.size() * sizeof(float));
		}
		if (UI::ActiveNoiseMap == 2 || !(mT0)) {
			Texture2_UploadPixels(mNoiseMapTexture2, perlinBuffer2.data(), perlinBuffer2.size() * sizeof(float));
		}
		if (!mT0) {
			mT0 = new Terrain(w, h, w / 8, h / 8);
			glm::mat4 transform = glm::scale(glm::mat4(1.0), glm::vec3(5.0, 2.5, 5.0)) * mT0->GetTransform();
			mT0->SetTransform(transform);
		}
		Texture2_ReadPixels(mNoiseMapTexture, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sizeof(float), perlinBuffer.data());
		Texture2_ReadPixels(mNoiseMapTexture1, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sizeof(float), perlinBuffer1.data());
		Texture2_ReadPixels(mNoiseMapTexture2, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sizeof(float), perlinBuffer2.data());
		mT0->ApplyHeightmap(hw, hh, -6.0, 25.0f, { {UI::Contribution[0], perlinBuffer.data() }, {UI::Contribution[1], perlinBuffer1.data() }, {UI::Contribution[2], perlinBuffer2.data() } });
		if (mMinimap) {
			Texture2_Destroy(mMinimap);
			mMinimap = mGeoPass->CreateMinimap();
			mUI->SetMinimap(mMinimap);
		}
	}

}