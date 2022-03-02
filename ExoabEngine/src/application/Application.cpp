#include "Application.hpp"
#include "../physics/Physics.hpp"
#include "../mesh/Map.hpp"
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
static constexpr const char* s_ShaderCache = "assets/shaders/spirv_cache/";
#else
static constexpr bool s_DebugMode = false;
static constexpr const char* s_ShaderCache = "assets/shaders/spirv_cacheoptimized/";
#endif
static constexpr const char* s_FramebufferReserve = "assets/materials/framebuffer_reserve.cfg";
static constexpr bool s_EnableImGui = true;
static constexpr uint32_t s_MaxObjects = 10;
static constexpr uint32_t s_Range = 10;

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
	IMaterialFramebuffer gFBO0;
	Material* gMaterial0;
	Material* gMapMaterial;
	ShaderTypes::TerrainTransform* gMapUBO[gFrameOverlapCount];
	IBuffer2 gMapVertices, gMapIndices;
	uint32_t gMapIndicesCount;
	VkSampler gSampler0;
	ITexture2 gWoodTex;
	Camera gCamera({ 0,0,0 });
	Camera gCamera2({0,0,0});
	static VkQueryPool s_Query[gFrameOverlapCount];
	static bool s_QueryWrite = true;
	
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
	static uint32_t* s_CulledDrawCount[gFrameOverlapCount];

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
		{ "assets/mesh/ball.obj",
		 "assets/mesh/ball.obj"
		}, gGeomtry, &gVerticesSSBO, &gIndicesBuffer);
	if (!MeshLoadStatus)
		return false;

	gECS = new EntityController(gGeomtry, s_MaxObjects, s_MaxObjects);

	s_Entites.count = s_MaxObjects;
	s_Entites.ent = new Entity[s_Entites.count];
	
	srand(42);
	int range = s_Range;
	for (unsigned int i = 0; i < s_MaxObjects; i++)
	{
		s_Entites.ent[i].m_geometryID = EntityGeometryID(rand() % 2);
		s_Entites.ent[i].m_objData.bounding_sphere_center = glm::vec3(0.0);
		s_Entites.ent[i].m_objData.bounding_sphere_radius = 1.0;
		s_Entites.ent[i].m_objData.m_Model = glm::translate(glm::mat4(1.0), glm::vec3((rand() % range) - (range / 2), (rand() % range) - (range / 2), (rand() % range) - (range / 2)));
		//s_Entites.ent[i].m_objData.m_Model = glm::translate(glm::mat4(1.0), glm::vec3(0, -5, -2));
		s_Entites.ent[i].m_objData.m_NormalModel = glm::mat4(1.0);
		s_Entites.ent[i].m_textureID = 0;
		gECS->AddEntity(&s_Entites.ent[i]);
	}

	//physx::PxMaterial* mat = Physx_CreateMaterial(0.5, 0.5, 0.6);
	//physx::PxTriangleMesh* gmesh = Physx_CreateTriangleMesh(gGeomtry[s_Entites.ent[0].m_geometryID]);
	//
	//physx::PxRigidDynamic* actor = Physx_CreateDynamicActor(mat, gmesh, { 2, 2, 2 }, physx::PxTransform());
	//physx::PxRigidBodyExt::updateMassAndInertia(*actor, 10.0f);
	//gScene->addActor(*actor);

	//mat->release();
	//gmesh->release();
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
	bindings[0].m_hostvisible = false;
	bindings[0].m_useclientbuffer = true;
	bindings[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].m_size = 0;
	bindings[0].m_client_buffer = gVerticesSSBO;
	
	bindings[1] = bindings[0];
	bindings[1].m_useclientbuffer = false;
	bindings[1].m_hostvisible = true;
	bindings[1].m_type = SHADER_BINDING_UNIFORM_BUFFER;
	bindings[1].m_bindingID = 1;
	bindings[1].m_size = sizeof(ShaderTypes::SceneData);
	
	bindings[2] = bindings[1];
	bindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	bindings[2].m_bindingID = 2;
	bindings[2].m_hostvisible = true;
	bindings[2].m_size = s_MaxObjects * sizeof(ShaderTypes::ObjectData);
	
	bindings[3] = bindings[2];
	bindings[3].m_bindingID = 3;
	bindings[3].m_hostvisible = false;
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

	/***** COMPUTE (Frustrum Culling) Pipeline  *****/

	std::vector<ShaderBinding> computeBindings(4);
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
	computeBindings[1].m_preinitalized  = false;
	computeBindings[1].m_additional_buffer_flags = (BufferType)0;
	computeBindings[1].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[1].m_size = bindings[3].m_size;

	computeBindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	computeBindings[2].m_bindingID = 2;
	computeBindings[2].m_hostvisible = false;
	computeBindings[2].m_useclientbuffer = false;
	computeBindings[2].m_preinitalized = true;
	computeBindings[2].m_additional_buffer_flags = (BufferType)0;
	computeBindings[2].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[2].m_ssbo = bindings[3].m_ssbo;

	computeBindings[3].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	computeBindings[3].m_bindingID = 3;
	computeBindings[3].m_hostvisible = true;
	computeBindings[3].m_useclientbuffer = false;
	computeBindings[3].m_preinitalized = false;
	computeBindings[3].m_additional_buffer_flags = BufferType::IndirectBuffer;
	computeBindings[3].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[3].m_size = sizeof(uint32_t);

	std::vector<VkDescriptorPoolSize> poolSize;
	ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSize, &computeBindings);
	computeSetPool = vk::Gfx_CreateDescriptorPool(gContext, 1 * gFrameOverlapCount, poolSize);
	computeSet0 = ShaderBinding_Create(gContext, computeSetPool, 0, &computeBindings);
	struct FrustrumPlanes
	{
		uint32_t u_MaxDraw[4];
		glm::vec4 u_CullPlanes[5];
	};
	computeLayout = ShaderBinding_CreatePipelineLayout(gContext, { computeSet0 }, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FrustrumPlanes)}});
	Shader computeShader = Shader(gContext, "assets/shaders/FrustrumCulling.comp");
	computeFrustrum = Pipeline_CreateCompute(gContext, &computeShader, computeLayout, 0);

	/********* Map Material **********/

	Map testingMap = Map_Create(100, 4, 100, 4);
	gMapIndicesCount = testingMap.m_indices.size();
	gMapVertices = Buffer2_Create(gContext, BufferType::StorageBuffer, testingMap.m_vertices.size() * sizeof(MapVertex), BufferMemoryType::GPU_ONLY);
	gMapIndices = Buffer2_Create(gContext, BufferType::IndexBuffer, testingMap.m_indices.size() * sizeof(uint32_t), BufferMemoryType::GPU_ONLY);
	Buffer2_UploadData(gMapVertices, (char8_t*)testingMap.m_vertices.data(), 0, VK_WHOLE_SIZE);
	Buffer2_UploadData(gMapIndices, (char8_t*)testingMap.m_indices.data(), 0, VK_WHOLE_SIZE);

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

	gMapMaterial = Material_Create(gContext, &basicmc, gFBOStateManagment0, nullptr, "assets/shaders/Terrain.vert", "assets/shaders/Terrain.frag", { {0, & terrainBindings}, {1, &terrainFragmentBinding} }, {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec3)}});

	for (int i = 0; i < gFrameOverlapCount; i++)
	{
		s_Query[i] = vk::Gfx_CreateQueryPool(gContext, VK_QUERY_TYPE_TIMESTAMP, 4, 0);
		s_ScenePtr[i] = (ShaderTypes::SceneData*)Buffer2_Map(bindings[1].m_buffer[i]);
		gObjectData[i] = (ShaderTypes::ObjectData*)Buffer2_Map(bindings[2].m_ssbo[i]);
		gDraws[i] = (ShaderTypes::DrawData*)Buffer2_Map(computeSet0->m_bindings[1].m_ssbo[i]);
		gMapUBO[i] = (ShaderTypes::TerrainTransform*)Buffer2_Map(gMapMaterial->m_sets[0]->m_bindings[1].m_buffer[i]);
		s_CulledDrawCount[i] = (uint32_t*)Buffer2_Map(computeSet0->m_bindings[3].m_ssbo[i]);
		gECS->UpdateDrawCommandAndObjectDataBuffer(gDraws[i], gObjectData[i]);
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
		shaderReadBarrier[i].image = gFBO0->m_textures[0]->m_vk_images_per_frame[i];
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
		Material_RecreatePipeline(gMapMaterial, spec);
	}

	const auto &frame = Graphics3D_GetFrame(gGfx);

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

	Buffer2_Flush(gMaterial0->m_sets[0]->m_bindings[1].m_buffer[frame.m_FrameIndex], 0, VK_WHOLE_SIZE);
	Buffer2_Flush(gMaterial0->m_sets[0]->m_bindings[2].m_buffer[frame.m_FrameIndex], 0, VK_WHOLE_SIZE);
	Buffer2_Flush(computeSet0->m_bindings[1].m_buffer[frame.m_FrameIndex], 0, VK_WHOLE_SIZE);

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
	
	/**** ==================FRUSTRUM CULLING================== ****/
	vkCmdFillBuffer(cmd, computeSet0->m_bindings[3].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer, 0, sizeof(uint32_t), 0);

	vkCmdResetQueryPool(cmd, s_Query[FrameIndex], 0, 4);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, s_Query[FrameIndex], 0);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computeFrustrum);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computeLayout, 0, 1, &computeSet0->m_set[FrameIndex], 0, nullptr);
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

	int temp = (gECS->GetDrawCount() + 63) / 64;
	vkCmdPushConstants(cmd, computeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(computePushblock), &computePushblock);
	vkCmdDispatch(cmd, temp, 1, 1);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, s_Query[FrameIndex], 1);

	VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = computeSet0->m_bindings[1].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer;
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	/**** ==================FRUSTRUM CULLING================== ****/

	//vkCmdBeginRenderPass(cmd, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkRenderingInfo renderingInfo;
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = nullptr;
	renderingInfo.flags = 0;
	renderingInfo.renderArea = BeginInfo.renderArea;
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;

	VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
	colorAttachment.imageView = gFBO0->m_textures[0]->m_vk_views_per_frame[FrameIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	colorAttachment.resolveImageView = nullptr;
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color = attachments[0].clearColor.color;
	VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthAttachment.imageView = gFBO0->m_textures[1]->m_vk_views_per_frame[FrameIndex];
	depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	depthAttachment.resolveImageView = nullptr;
	depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.clearValue.depthStencil = gMaterial0->m_state_managment->m_depth_attachment.value().clearColor.depthStencil;

	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthAttachment;
	renderingInfo.pStencilAttachment = nullptr;
	vkCmdBeginRendering(cmd, &renderingInfo);
	VkImageMemoryBarrier renderBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	renderBarrier.srcAccessMask = VK_ACCESS_NONE;
	renderBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	renderBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	renderBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	renderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	renderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	renderBarrier.image = gFBO0->m_textures[0]->m_vk_images_per_frame[FrameIndex];
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
	depthBarrier.image = gFBO0->m_textures[1]->m_vk_images_per_frame[FrameIndex];
	depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthBarrier.subresourceRange.layerCount = 1;
	depthBarrier.subresourceRange.levelCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &renderBarrier);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthBarrier);

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, s_Query[FrameIndex], 2);
	
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial0->m_pipeline_state->m_pipeline);
	VkDescriptorSet sets[2] = { gMaterial0->m_sets[0]->m_set[FrameIndex], gMaterial0->m_sets[1]->m_set[FrameIndex]};
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMaterial0->m_layout, 0, 2, sets, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, gIndicesBuffer->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirectCount(cmd, gMaterial0->m_sets[0]->m_bindings[3].m_ssbo[FrameIndex]->m_vk_buffer->m_buffer, 0, computeSet0->m_bindings[3].m_buffer[FrameIndex]->m_vk_buffer->m_buffer, 0, s_MaxObjects, sizeof(ShaderTypes::DrawData));

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMapMaterial->m_pipeline_state->m_pipeline);
	sets[0] = gMapMaterial->m_sets[0]->m_set[FrameIndex];
	sets[1] = gMapMaterial->m_sets[1]->m_set[FrameIndex];
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gMapMaterial->m_layout, 0, 2, sets, 0, nullptr);
	vkCmdBindIndexBuffer(cmd, gMapIndices->m_vk_buffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	const glm::vec3 LightDir = glm::vec3(0.0, -0.5, 0.5);
	vkCmdPushConstants(cmd, gMapMaterial->m_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec3), &LightDir);
	vkCmdDrawIndexed(cmd, gMapIndicesCount, 1, 0, 0, 0);

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_Query[FrameIndex], 3);

	VkImageMemoryBarrier presentBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	presentBarrier.dstAccessMask = VK_ACCESS_NONE;
	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	presentBarrier.image = gFBO0->m_textures[0]->m_vk_images_per_frame[FrameIndex];
	presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentBarrier.subresourceRange.layerCount = 1;
	presentBarrier.subresourceRange.levelCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);

	vkCmdEndRendering(cmd);
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
	for(int i = 0; i < gFrameOverlapCount; i++)
	vkDestroyQueryPool(gContext->defaultDevice, s_Query[i], nullptr);
	vkDestroyDescriptorPool(gContext->defaultDevice, computeSetPool, nullptr);
	ShaderBinding_DestroySets(gContext, { computeSet0 });
	vkDestroyPipelineLayout(gContext->defaultDevice, computeLayout, nullptr);
	vkDestroyPipeline(gContext->defaultDevice, computeFrustrum, nullptr);
	Material_DestroyFramebuffer(gFBO0);
	Material_Destory(gMaterial0);
	Material_Destory(gMapMaterial);
	Buffer2_Destroy(gMapVertices);
	Buffer2_Destroy(gMapIndices);
	vkDestroySampler(gContext->defaultDevice, gSampler0, nullptr);
	Texture2_Destroy(gWoodTex);
	Buffer2_Destroy(gVerticesSSBO);
	Buffer2_Destroy(gIndicesBuffer);
	delete gFramebufferReserve;
	Graphics3D_Destroy(gGfx);
	Physx_Destroy();
	delete[] s_Entites.ent;
	delete gECS;
	glfwTerminate();
}
