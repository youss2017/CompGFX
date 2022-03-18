#include "FrustrumCullPass.hpp"
#include <shaders/Shader.hpp>
#include <pipeline/PipelineCS.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../mesh/geometry.hpp"
#include "../../window/PlatformWindow.hpp"

extern vk::VkContext gContext;

namespace Application {
	extern PlatformWindow* gWindow;
	extern std::vector<Mesh::Geometry> gGeomtry;

	struct FrustrumPlanes {
		int u_MaxGeometry;
		glm::vec4 u_CullPlanes[5];
	};
}

Application::FrustumCullPass::FrustumCullPass(EntityController* ecs, Camera* camera) : Scene(gContext->defaultDevice), mECS(ecs), mCamera(camera)
{
	std::vector<ShaderBinding> computeBindings(5);
	computeBindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	computeBindings[0].m_bindingID = 0;
	computeBindings[0].m_preinitalized = true;
	computeBindings[0].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[0].m_ssbo = ecs->GetGeometryDataArray();

	computeBindings[1].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	computeBindings[1].m_bindingID = 1;
	computeBindings[1].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[1].m_size = ecs->GetGeometryDataArray()[0]->size;

	computeBindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	computeBindings[2].m_bindingID = 2;
	computeBindings[2].m_preinitalized = true;
	computeBindings[2].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[2].m_ssbo = ecs->GetDrawDataArray();

	computeBindings[3].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	computeBindings[3].m_bindingID = 3;
	computeBindings[3].m_hostvisible = true;
	computeBindings[3].m_useclientbuffer = false;
	computeBindings[3].m_preinitalized = false;
	computeBindings[3].m_additional_buffer_flags = BufferType(BUFFER_TYPE_INDIRECT | BUFER_TYPE_TRANSFER_SRC);
	computeBindings[3].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[3].m_size = ecs->GetDrawDataArray()[0]->size;

	computeBindings[4].m_type = SHADER_BINDING_UNIFORM_BUFFER;
	computeBindings[4].m_bindingID = 4;
	computeBindings[4].m_hostvisible = true;
	computeBindings[4].m_useclientbuffer = false;
	computeBindings[4].m_preinitalized = false;
	computeBindings[4].m_additional_buffer_flags = BufferType(0);
	computeBindings[4].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[4].m_size = sizeof(FrustrumPlanes);

	std::vector<VkDescriptorPoolSize> poolSize;
	ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSize, &computeBindings);
	mPool = vk::Gfx_CreateDescriptorPool(gContext, 1 * gFrameOverlapCount, poolSize);
	mSet = ShaderBinding_Create(gContext, mPool, 0, &computeBindings);

	mFrustrumLayout = ShaderBinding_CreatePipelineLayout(gContext, { mSet }, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FrustrumPlanes)} });
	Shader computeShader = Shader(gContext, "assets/shaders/FrustrumCulling.comp");
	mFrustrum = Pipeline_CreateCompute(gContext, &computeShader, mFrustrumLayout, 0);

	mOutputGeometryDataArray = mSet->m_bindings[1].m_ssbo;
	mOutputDrawDataArray = mSet->m_bindings[3].m_ssbo;

	for (int i = 0; i < gFrameOverlapCount; i++) {
		mFrustrumPlanesMapped[i] = Buffer2_Map(mSet->GetBuffer2(4, i));
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		VkCommandBuffer cmd = mCmds[i];
		vkBeginCommandBuffer(cmd, &beginInfo);

		VkBuffer outputDrawDataBuffer = mOutputDrawDataArray[i]->m_vk_buffer->m_buffer;
		vkCmdFillBuffer(cmd, outputDrawDataBuffer, 0, VK_WHOLE_SIZE, 0);
		VkBufferMemoryBarrier drawBufferBarrier = vk::Gfx_BufferMemoryBarrier(VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, outputDrawDataBuffer);
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &drawBufferBarrier, 0, nullptr);
		VkBufferMemoryBarrier computeVertexShaderBarrier[2] = {
			vk::Gfx_BufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, mOutputGeometryDataArray[i]->m_vk_buffer->m_buffer),
			vk::Gfx_BufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, mOutputDrawDataArray[i]->m_vk_buffer->m_buffer)
		};
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 2, computeVertexShaderBarrier, 0, nullptr);

		VkDescriptorSet computeSets[1] = { mSet->m_set[i] };
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mFrustrum);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mFrustrumLayout, 0, 1, computeSets, 0, nullptr);
		int drawCount = mECS->GetDrawCount();
		int instanceCount = mECS->GetInstanceCount();

		vkCmdDispatch(cmd, (instanceCount + 63) / 64, (drawCount + 7) / 8, 1);

		vkEndCommandBuffer(cmd);
	}
}

Application::FrustumCullPass::~FrustumCullPass()
{
	for (int i = 0; i < gFrameOverlapCount; i++) {
		vkDestroyCommandPool(mDevice, mPools[i], nullptr);
	}
	vkDestroyPipelineLayout(mDevice, mFrustrumLayout, nullptr);
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	ShaderBinding_DestroySets(gContext, { mSet });
	vkDestroyPipeline(mDevice, mFrustrum, nullptr);
}

void Application::FrustumCullPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart)
{
	glm::mat4 projT = glm::transpose(mCamera->GetViewMatrix() * glm::perspectiveFovLH(glm::radians(90.0f), (float)gWindow->m_width, float(gWindow->m_height), 0.1f, 1000.0f));
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
	memcpy(mFrustrumPlanesMapped[FrameIndex], &planes, sizeof(planes));
	Buffer2_Flush(mSet->GetBuffer2(4, FrameIndex), 0, VK_WHOLE_SIZE);
}

VkCommandBuffer Application::FrustumCullPass::Frame(uint32_t FrameIndex)
{
	return mCmds[FrameIndex];
}
