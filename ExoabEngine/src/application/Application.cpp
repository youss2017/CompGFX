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
	//Material* gMaterial1;
	Material* gMapMaterial;
	ShaderTypes::TerrainTransform* gMapUBO[gFrameOverlapCount];
	IBuffer2 gMapVertices, gMapIndices;
	VkSampler gSampler0;
	ITexture2 gWoodTex;
	Camera gCamera({ 0,0,0 });
	Camera gCamera2({0,0,0});
	static VkQueryPool s_Query[gFrameOverlapCount];
	static bool s_QueryWrite = true;
	Framebuffer gFBO0;
	Map testingMap;
	
	EntityController* gECS;
	static FrustrumCompute frustrumCompute;

	// Shader Data
	// !!! IMPORTANT !!! Write ONLY by using memcpy
	// using the '=' operator will cause a read operation, since were using coherent memory
	// that means read from GPU memory to CPU memory
	static ShaderTypes::SceneData* s_ScenePtr[gFrameOverlapCount];
	static uint32_t* s_CulledDrawCount[gFrameOverlapCount];

	//static IBuffer2 sAnimVertices, sAnimIndices;
	//static std::vector<Mesh::SkinnedMesh> skinnedMeshes;
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
		{ "assets/mesh/Ball.obj",
		  "assets/mesh/CmdHQ.fbx",
		}, gGeomtry, &gVerticesSSBO, &gIndicesBuffer);
	if (!MeshLoadStatus)
		return false;

	//Mesh::LoadVerticesIndicesBONE(gContext, { "assets/mesh/horse.dae" }, skinnedMeshes, &sAnimVertices, &sAnimIndices);

	gECS = new EntityController(gGeomtry);

	srand(140);
	int range = s_Range;
	for (unsigned int i = 0; i < s_MaxObjects; i++)
	{
		IEntity ent = NewEntity();
		ent->m_geometryID = EntityGeometryID(rand() % 2);
		ent->m_objData.bounding_sphere_center = glm::vec3(0.0);
		ent->m_objData.bounding_sphere_radius = 1.0;
		ent->m_objData.m_Model = glm::translate(glm::mat4(1.0), glm::vec3((rand() % range) - (range / 2), (rand() % range) - (range / 2), (rand() % range) - (range / 2)));
		ent->m_objData.m_Model *= glm::scale(glm::mat4(1.0), glm::vec3(0.005f));
		//s_Entites.ent[i].m_objData.m_Model = glm::translate(glm::mat4(1.0), glm::vec3(0, -5, -2));
		ent->m_objData.m_NormalModel = glm::mat4(1.0);
		ent->m_textureID = 0;
		gECS->AddEntity(ent);
	}

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
	
	std::vector<ShaderBinding> bindings(3);
	bindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[0].m_bindingID = 0;
	bindings[0].m_hostvisible = false;
	bindings[0].m_useclientbuffer = true;
	bindings[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].m_size = 0;
	bindings[0].m_client_buffer = gVerticesSSBO;

	bindings[1].m_type = SHADER_BINDING_UNIFORM_BUFFER;
	bindings[1].m_bindingID = 1;
	bindings[1].m_hostvisible = true;
	bindings[1].m_useclientbuffer = false;
	bindings[1].m_preinitalized = false;
	bindings[1].m_additional_buffer_flags  = (BufferType)0;
	bindings[1].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[1].m_size = sizeof(ShaderTypes::SceneData);
	/*
	bindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[2].m_bindingID = 2;
	bindings[2].m_hostvisible = true;
	bindings[2].m_useclientbuffer = false;
	bindings[2].m_preinitalized = true;
	bindings[2].m_additional_buffer_flags = (BufferType)0;
	bindings[2].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[2].m_size = gECS->GetDrawCount() * sizeof(ShaderTypes::ObjectData);
	bindings[2].m_ssbo = gECS->GetObjectBufferArray();
	*/
	bindings[2/*3*/].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[2/*3*/].m_bindingID = 3;
	bindings[2/*3*/].m_hostvisible = true;
	bindings[2/*3*/].m_useclientbuffer = false;
	bindings[2/*3*/].m_preinitalized = false;
	bindings[2/*3*/].m_additional_buffer_flags = BufferType::IndirectBuffer;
	bindings[2/*3*/].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[2/*3*/].m_size = gECS->GetDrawCount() * sizeof(ShaderTypes::DrawData);

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

	VkPushConstantRange vertexRange{};
	vertexRange.offset = 0;
	vertexRange.size = 8;
	vertexRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	gMaterial0 = Material_Create(gContext, gFBO0, &basicmc, nullptr, { {0, &bindings}, {1, &set1Bindings} }, {vertexRange});

	/***** COMPUTE (Frustrum Culling) Pipeline  *****/

	frustrumCompute = Application::CreateFrustrumCompute(gECS->GetObjectBufferArray(), gECS->GetDrawBufferArray(), bindings[2/*3*/].m_ssbo);
	
	/********* Map Material **********/

	testingMap = Map_Create(10, 1, 10, 1, 5);
	gMapVertices = Buffer2_Create(gContext, BufferType::StorageBuffer, testingMap.m_totalVerticesCount * sizeof(MapVertex), BufferMemoryType::GPU_ONLY);
	gMapIndices = Buffer2_Create(gContext, BufferType::IndexBuffer, testingMap.m_totalIndicesCount * sizeof(uint32_t), BufferMemoryType::GPU_ONLY);
	Buffer2_UploadData(gMapVertices, (char8_t*)testingMap.m_vertices.data(), 0, testingMap.m_vertices.size() * sizeof(MapVertex));
	Buffer2_UploadData(gMapIndices, (char8_t*)testingMap.m_indices.data(), 0, testingMap.m_indices.size() * sizeof(uint32_t));

	std::vector<ShaderBinding> terrainBindings(2);
	terrainBindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	terrainBindings[0].m_bindingID = 0;
	terrainBindings[0].m_hostvisible = false;
	terrainBindings[0].m_useclientbuffer = true;
	terrainBindings[0].m_preinitalized = false;
	terrainBindings[0].m_additional_buffer_flags = (BufferType)0;
	terrainBindings[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	terrainBindings[0].m_size = gMapVertices->size;
	terrainBindings[0].m_client_buffer = gMapVertices;

	terrainBindings[1].m_type = SHADER_BINDING_UNIFORM_BUFFER;
	terrainBindings[1].m_bindingID = 1;
	terrainBindings[1].m_hostvisible = true;
	terrainBindings[1].m_useclientbuffer = false;
	terrainBindings[1].m_preinitalized = false;
	terrainBindings[1].m_additional_buffer_flags = (BufferType)0;
	terrainBindings[1].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	terrainBindings[1].m_size = sizeof(ShaderTypes::TerrainTransform);

	std::vector<ShaderBinding> terrainFragmentBinding(1);
	terrainFragmentBinding[0].m_type = SHADER_BINDING_COMBINED_TEXTURE_SAMPLER;
	terrainFragmentBinding[0].m_bindingID = 0;
	terrainFragmentBinding[0].m_hostvisible = false;
	terrainFragmentBinding[0].m_useclientbuffer = false;
	terrainFragmentBinding[0].m_preinitalized  = false;
	terrainFragmentBinding[0].m_additional_buffer_flags = (BufferType)0;
	terrainFragmentBinding[0].m_shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	terrainFragmentBinding[0].m_size = 0;
	terrainFragmentBinding[0].m_sampler.push_back(gSampler0);
	terrainFragmentBinding[0].m_textures.push_back(gWoodTex);
	terrainFragmentBinding[0].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	gMapMaterial = Material_Create(gContext, gFBO0, &basicmc, nullptr, "assets/shaders/Terrain.vert", "assets/shaders/Terrain.frag", { {0, &terrainBindings}, {1, &terrainFragmentBinding} }, {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec3)}});
#if 0
	std::vector<ShaderBinding> bindings1(3);
	bindings1[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings1[0].m_bindingID = 0;
	bindings1[0].m_hostvisible = false;
	bindings1[0].m_useclientbuffer = true;
	bindings1[0].m_preinitalized = false;
	bindings1[0].m_additional_buffer_flags = (BufferType)0;
	bindings1[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings1[0].m_size = 0;
	bindings1[0].m_client_buffer = sAnimVertices;

	bindings1[1].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings1[1].m_bindingID = 1;
	bindings1[1].m_hostvisible = true;
	bindings1[1].m_useclientbuffer = false;
	bindings1[1].m_preinitalized = false;
	bindings1[1].m_additional_buffer_flags = (BufferType)0;
	bindings1[1].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings1[1].m_size = sizeof(ShaderTypes::ObjectDataBONE);

	bindings1[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings1[2].m_bindingID = 2;
	bindings1[2].m_hostvisible = true;
	bindings1[2].m_useclientbuffer = false;
	bindings1[2].m_preinitalized = false;
	bindings1[2].m_additional_buffer_flags = BufferType::IndirectBuffer;
	bindings1[2].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings1[2].m_size = sizeof(ShaderTypes::DrawCommand);

	gMaterial1 = Material_Create(gContext, gFBO0, &basicmc, nullptr, "assets/shaders/AniVertex.vert", "assets/shaders/fragment2.frag", { {0, &bindings1} }, {});
#endif
	for (int i = 0; i < gFrameOverlapCount; i++)
	{
		s_Query[i] = vk::Gfx_CreateQueryPool(gContext, VK_QUERY_TYPE_TIMESTAMP, 4, 0);
		s_ScenePtr[i] = (ShaderTypes::SceneData*)Buffer2_Map(bindings[1].m_buffer[i]);
		gMapUBO[i] = (ShaderTypes::TerrainTransform*)Buffer2_Map(gMapMaterial->m_sets[0]->m_bindings[1].m_buffer[i]);
		s_CulledDrawCount[i] = (uint32_t*)Buffer2_Map(frustrumCompute.m_set0->m_bindings[3].m_ssbo[i]);
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

	if (UI::StateChanged)
	{
		UI::StateChanged = false;
		auto spec = gMaterial0->m_pipeline_state->m_spec;
		spec.m_PolygonMode = UI::ShowWireframe ? PolygonMode::LINE : PolygonMode::FILL;
		Graphics3D_WaitGPUIdle(gGfx);
		Material_RecreatePipeline(gMaterial0, spec);
		//Material_RecreatePipeline(gMaterial1, spec);
		Material_RecreatePipeline(gMapMaterial, spec);
	}
	const auto &frame = Graphics3D_GetFrame(gGfx);
	gECS->Prepare(frame.m_FrameIndex);

	glm::mat4 proj = glm::perspective(90.0f, gWindow->m_width / float(gWindow->m_height), 0.1f, 1000.0f);
	glm::mat4 view = gCamera.GetViewMatrix();
	memcpy(&s_ScenePtr[frame.m_FrameIndex]->m_Projection, &proj, sizeof(glm::mat4));
	memcpy(&s_ScenePtr[frame.m_FrameIndex]->m_View, &view, sizeof(glm::mat4));
	ShaderTypes::TerrainTransform terrainT;
	terrainT.u_Model = glm::scale(glm::mat4(1.0), glm::vec3(5.0, 5.0, 5.0));
	terrainT.u_NormalModel = glm::transpose(glm::inverse(terrainT.u_Model));
	terrainT.u_View = view;
	terrainT.u_Projection = proj;
	memcpy(gMapUBO[frame.m_FrameIndex], &terrainT, sizeof(ShaderTypes::TerrainTransform));

	uint64_t results[4];
	VkResult qs = vkGetQueryPoolResults(gContext->defaultDevice, s_Query[frame.m_FrameIndex], 0, 4, sizeof(results), results, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	if (qs == VK_SUCCESS && UpdateUIInfo)
	{
		UI::FrustrumCullingTime = (double(results[1] - results[0]) * gContext->card.deviceLimits.timestampPeriod) * 1e-6;
		UI::GPUPassTime = (double(results[3] - results[2]) * gContext->card.deviceLimits.timestampPeriod) * 1e-6;
		UI::FrameRate = FrameRate;
		UI::FrameTime = dTime * 1e3;
	}
	UI::InputDrawCount = double(gECS->GetDrawCount());
	UI::OutputDrawCount = double(*s_CulledDrawCount[frame.m_FrameIndex]);

	//Physx_Simulate();

	return true;
}


namespace Application {
	void FrustrumCulling(VkCommandBuffer cmd, uint32_t FrameIndex) {
		/**** ==================FRUSTRUM CULLING================== ****/
		vkCmdFillBuffer(cmd, frustrumCompute.m_set0->m_bindings[3].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer, 0, sizeof(uint32_t), 0);

		vkCmdResetQueryPool(cmd, s_Query[FrameIndex], 0, 4);
		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, s_Query[FrameIndex], 0);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, frustrumCompute.m_pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, frustrumCompute.m_layout, 0, 1, &frustrumCompute.m_set0->m_set[FrameIndex], 0, nullptr);
		struct {
			uint32_t u_MaxDraw[4];
			glm::vec4 u_FrustrumPlanes[5];
		} computePushblock;

		if (!UI::LockFrustrum)
			memcpy(&gCamera2, &gCamera, sizeof(Camera));
		glm::mat4 proj = glm::transpose(glm::perspective(90.0f, gWindow->m_width / float(gWindow->m_height), 0.1f, 1000.0f) * gCamera2.GetViewMatrix());
		auto normalizePlane = [](glm::vec4 p) -> glm::vec4
		{
			return p / glm::length(glm::vec3(p));
		};
		computePushblock.u_FrustrumPlanes[0] = normalizePlane(proj[3] + proj[0]);
		computePushblock.u_FrustrumPlanes[1] = normalizePlane(proj[3] - proj[0]);
		computePushblock.u_FrustrumPlanes[2] = normalizePlane(proj[3] + proj[1]);
		computePushblock.u_FrustrumPlanes[3] = normalizePlane(proj[3] - proj[1]);
		computePushblock.u_FrustrumPlanes[4] = normalizePlane(proj[3] - proj[2]);
		computePushblock.u_MaxDraw[0] = gECS->GetDrawCount();

		int temp = (gECS->GetDrawCount() + 31) / 32;
		vkCmdPushConstants(cmd, frustrumCompute.m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(computePushblock), &computePushblock);
		vkCmdDispatch(cmd, temp, 1, 1);
		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, s_Query[FrameIndex], 1);

		VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = frustrumCompute.m_set0->m_bindings[1].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
		/**** ==================FRUSTRUM CULLING================== ****/
	}

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
	//auto& attachments = gMaterial0->m_state_managment->m_attachments;
	VkClearValue pClearValues[2];
	pClearValues[0] = { 0, 0, 0, 0 };
	pClearValues[1] = { 1.0 };
	
	FrustrumCulling(cmd, FrameIndex);
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

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, s_Query[FrameIndex], 2);
	
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial0->m_pipeline_state->m_pipeline);
	VkDescriptorSet sets[2] = { gMaterial0->m_sets[0]->m_set[FrameIndex], gMaterial0->m_sets[1]->m_set[FrameIndex]};
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial0->m_layout, 0, 2, sets, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, gIndicesBuffer->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	
	uint64_t gpupointer = Buffer2_GetGPUPointer(gECS->GetObjectBuffer(FrameIndex));
	vkCmdPushConstants(cmd, gMaterial0->m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 8, &gpupointer);
	vkCmdDrawIndexedIndirectCount(cmd, gMaterial0->m_sets[0]->m_bindings[/*3*/2].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer, 0, frustrumCompute.m_set0->m_bindings[3].m_buffer[FrameIndex]->m_vk_buffer->m_buffer, 0, gECS->GetDrawCount(), sizeof(ShaderTypes::DrawData));

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMapMaterial->m_pipeline_state->m_pipeline);
	sets[0] = gMapMaterial->m_sets[0]->m_set[FrameIndex];
	sets[1] = gMapMaterial->m_sets[1]->m_set[FrameIndex];
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMapMaterial->m_layout, 0, 2, sets, 0, nullptr);
	const glm::vec3 LightDir = glm::vec3(0.0, -0.5, 0.5);
	vkCmdPushConstants(cmd, gMapMaterial->m_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec3), &LightDir);
	vkCmdBindIndexBuffer(cmd, gMapIndices->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmd, testingMap.m_indices.size(), 1, 0, 0, 0);
#if 0
	auto objData = (ShaderTypes::ObjectDataBONE*)Buffer2_Map(gMaterial1->m_sets[0]->m_bindings[1].m_ssbo[FrameIndex]);
	auto drawCommand = (ShaderTypes::DrawCommand*)Buffer2_Map(gMaterial1->m_sets[0]->m_bindings[2].m_ssbo[FrameIndex]);

	drawCommand[0].indexCount = sAnimIndices->size / sizeof(uint32_t);
	drawCommand[0].instanceCount = 1;
	drawCommand[0].firstIndex = 0;
	drawCommand[0].vertexOffset = 0;
	drawCommand[0].firstInstance = 0;
	drawCommand[0].objDataIndex = 0;

	objData[0].model = glm::translate(glm::mat4(1.0), glm::vec3(0, -5, 0)) * (glm::rotate(glm::mat4(1.0), glm::radians(270.0f), glm::vec3(0.0, 1.0, 0.0)) * glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(1, 0, 0)));
	objData[0].model *= glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01));
	objData[0].view = gCamera.GetViewMatrix();
	objData[0].projection = glm::perspective(90.0f, gWindow->m_width / float(gWindow->m_height), 0.1f, 1000.0f);
	std::vector<glm::mat4> finalTransforms;
	skinnedMeshes[0].GetFinalTransforms(finalTransforms);
	//memcpy(&objData[0].finalBoneTransformations[0], &finalTransforms[0], sizeof(glm::mat4)* finalTransforms.size());

	Buffer2_Flush(gMaterial1->m_sets[0]->m_bindings[1].m_ssbo[FrameIndex], 0, VK_WHOLE_SIZE);
	Buffer2_Flush(gMaterial1->m_sets[0]->m_bindings[2].m_ssbo[FrameIndex], 0, VK_WHOLE_SIZE);

	//Buffer2_Unmap(gMaterial1->m_sets[0]->m_bindings[1].m_ssbo[FrameIndex]);
	//Buffer2_Unmap(gMaterial1->m_sets[0]->m_bindings[2].m_ssbo[FrameIndex]);
	sets[0] = gMaterial1->m_sets[0]->m_set[FrameIndex];
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial1->m_pipeline_state->m_pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial1->m_layout, 0, 1, sets, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, sAnimIndices->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirect(cmd, gMaterial1->m_sets[0]->m_bindings[2].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer, 0, 1, sizeof(ShaderTypes::DrawCommand));
#endif

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_Query[FrameIndex], 3);

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
	for(int i = 0; i < gFrameOverlapCount; i++)
	vkDestroyQueryPool(gContext->defaultDevice, s_Query[i], nullptr);
	//Buffer2_Destroy(sAnimVertices);
	//Buffer2_Destroy(sAnimIndices);
	DestroyFrusturumCompute(frustrumCompute);
	Material_Destory(gMaterial0);
	//Material_Destory(gMaterial1);
	Material_Destory(gMapMaterial);
	Buffer2_Destroy(gMapVertices);
	Buffer2_Destroy(gMapIndices);
	gFBO0.DestroyAllBoundAttachments();
	vkDestroySampler(gContext->defaultDevice, gSampler0, nullptr);
	Texture2_Destroy(gWoodTex);
	Buffer2_Destroy(gVerticesSSBO);
	Buffer2_Destroy(gIndicesBuffer);
	Physx_Destroy();
	delete gECS;
	Graphics3D_Destroy(gGfx);
}
