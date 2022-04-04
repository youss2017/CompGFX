#include "FrustrumCullPass.hpp"
#include <shaders/Shader.hpp>
#include <pipeline/PipelineCS.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../mesh/geometry.hpp"
#include "../../window/PlatformWindow.hpp"
#include <backend/VkGraphicsCard.hpp>
#include "../Globals.hpp"

// Specialization Constants
// TODO: Use 32 for Nvidia Graphics cards
#define KernalSizeX 64
#define KernalSizeY 8
#define DisableCulling (false)
// Big objects may be completely outside one frustrum plane, however they may be partialy inside another frustrum plane
// the radius epsillion multiples the radius to reduce incorrect culling results, this is a hotfix.
#define RadiusEpsillion (2.5f)

namespace Application {
	struct FrustrumPlanes {
		int u_MaxGeometry;
		glm::vec4 u_CullPlanes[5];
	};
}

Application::FrustumCullPass::FrustumCullPass(EntityController* ecs, Camera* camera) : Pass(Global::Context->defaultDevice, true), mECS(ecs), mCamera(camera)
{
	BindingDescription computeBindings[5]{};
	computeBindings[0].mBindingID = 0;
	computeBindings[0].mFlags = 0;
	computeBindings[0].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeBindings[0].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[0].mBufferSize = 0;
	computeBindings[0].mBuffer = ecs->GetGeometryDataBuffer();
	computeBindings[0].mSharedResources = true;

	computeBindings[1].mBindingID = 1;
	computeBindings[1].mFlags = BINDING_FLAG_CREATE_BUFFERS_POINTER;
	computeBindings[1].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeBindings[1].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[1].mBufferSize = ecs->GetGeometryDataBuffer()->mSize;
	computeBindings[1].mBuffer = nullptr;

	computeBindings[2].mBindingID = 2;
	computeBindings[2].mFlags = 0;
	computeBindings[2].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeBindings[2].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[2].mBufferSize = 0;
	computeBindings[2].mBuffer = ecs->GetDrawDataBuffer();
	computeBindings[2].mSharedResources = true;

	computeBindings[3].mBindingID = 3;
	computeBindings[3].mFlags = BINDING_FLAG_BUFFER_USAGE_INDIRECT | BINDING_FLAG_CREATE_BUFFERS_POINTER;
	computeBindings[3].mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	computeBindings[3].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[3].mBufferSize = ecs->GetDrawDataBuffer()->mSize;
	computeBindings[3].mBuffer = nullptr;
	computeBindings[3].mSharedResources = false;

	computeBindings[4].mBindingID = 4;
	computeBindings[4].mFlags = BINDING_FLAG_CPU_VISIBLE;
	computeBindings[4].mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	computeBindings[4].mStages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeBindings[4].mBufferSize = sizeof(FrustrumPlanes);
	computeBindings[4].mBuffer = nullptr;

	std::vector<VkDescriptorPoolSize> poolSize;
	ShaderConnector_CalculateDescriptorPool(5, computeBindings, poolSize);
	mPool = vk::Gfx_CreateDescriptorPool(Global::Context, 1 * gFrameOverlapCount, poolSize);
	mSet = ShaderConnector_CreateSet(0, mPool, 5, computeBindings);

	mFrustrumLayout = ShaderConnector_CreatePipelineLayout(1, &mSet, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FrustrumPlanes)} });
	Shader computeShader = Shader(Global::Context, "assets/shaders/FrustrumCulling.comp");
	computeShader.SetSpecializationConstant<unsigned int>(0, KernalSizeX);
	computeShader.SetSpecializationConstant<unsigned int>(1, KernalSizeY);
	computeShader.SetSpecializationConstant<unsigned int>(2, DisableCulling);
	computeShader.SetSpecializationConstant<float>(3, RadiusEpsillion);
	mFrustrum = Pipeline_CreateCompute(Global::Context, &computeShader, mFrustrumLayout, 0);

	mOutputGeometryDataArray = mSet.GetBuffer2(1);
	mOutputDrawDataArray = mSet.GetBuffer2(3);

	mQuery = vk::Gfx_CreateQueryPool(Global::Context, VK_QUERY_TYPE_TIMESTAMP, gFrameOverlapCount * 2, 0);
	mInvocationQuery = vk::Gfx_CreateQueryPool(Global::Context, VK_QUERY_TYPE_PIPELINE_STATISTICS, gFrameOverlapCount, VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT);
	
	for (int i = 0; i < gFrameOverlapCount; i++) {
		RecordCommands(i);
	}
}

Application::FrustumCullPass::~FrustumCullPass()
{
	Super_Pass_Destroy();
	vkDestroyPipelineLayout(mDevice, mFrustrumLayout, nullptr);
	vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	ShaderConnector_DestroySet(mSet);
	vkDestroyPipeline(mDevice, mFrustrum, nullptr);
	vkDestroyQueryPool(mDevice, mQuery, nullptr);
	vkDestroyQueryPool(mDevice, mInvocationQuery, nullptr);
}

void Application::FrustumCullPass::ReloadShaders() {
	vkDestroyPipeline(mDevice, mFrustrum, nullptr);
	Shader computeShader = Shader(Global::Context, "assets/shaders/FrustrumCulling.comp");
	computeShader.SetSpecializationConstant<unsigned int>(0, KernalSizeX);
	computeShader.SetSpecializationConstant<unsigned int>(1, KernalSizeY);
	computeShader.SetSpecializationConstant<unsigned int>(2, DisableCulling);
	computeShader.SetSpecializationConstant<float>(3, RadiusEpsillion);
	mFrustrum = Pipeline_CreateCompute(Global::Context, &computeShader, mFrustrumLayout, 0);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		RecordCommands(i);
	}
}

VkCommandBuffer Application::FrustumCullPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart)
{
	glm::mat4 view = mCamera->GetViewMatrix();
	glm::mat4 projT = glm::transpose(Global::Projection * view);
	auto normalizePlane = [](glm::vec4 p) -> glm::vec4
	{
		return p / glm::length(glm::vec3(p));
	};
	FrustrumPlanes planes;
	planes.u_MaxGeometry = Global::Geomtry.size();
	planes.u_CullPlanes[0] = normalizePlane(projT[3] + projT[0]);
	planes.u_CullPlanes[1] = normalizePlane(projT[3] - projT[0]);
	planes.u_CullPlanes[2] = normalizePlane(projT[3] + projT[1]);
	planes.u_CullPlanes[3] = normalizePlane(projT[3] - projT[1]);
	planes.u_CullPlanes[4] = normalizePlane(projT[3] - projT[2]);
	memcpy(mFrustrumPlanesMapped[FrameIndex], &planes, sizeof(planes));
	Buffer2_Flush(mSet.GetBuffer2(4), 0, VK_WHOLE_SIZE);
	return *mCmd;
}

VkResult Application::FrustumCullPass::GetComputeShaderStatistics(bool Wait, uint32_t FrameIndex, double& duration, uint64_t& invocations)
{
	uint64_t data[2];
	uint64_t invoc;
	VkResult qr = vkGetQueryPoolResults(mDevice, mQuery, FrameIndex * 2, 2, sizeof(data), data, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (Wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	VkResult qr2 = vkGetQueryPoolResults(mDevice, mInvocationQuery, FrameIndex, 1, sizeof(uint64_t), &invoc, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | (Wait ? VK_QUERY_RESULT_WAIT_BIT : 0));
	if (qr == VK_SUCCESS) {
		duration = (double(data[1] - data[0]) * Global::Context->card.deviceLimits.timestampPeriod) * 1e-6;
		invocations = invoc;
		return VK_SUCCESS;
	}
	return VK_TIMEOUT;
}

void Application::FrustumCullPass::RecordCommands(uint32_t FrameIndex)
{
	mCmdPool->Reset(FrameIndex);
	uint32_t i = FrameIndex;
	mFrustrumPlanesMapped[i] = Buffer2_Map(mSet.GetBuffer2(4));
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkCommandBuffer cmd = mCmd->mCmds[FrameIndex];
	vkBeginCommandBuffer(cmd, &beginInfo);
	vkCmdResetQueryPool(cmd, mQuery, (i * 2), 2);
	vkCmdResetQueryPool(cmd, mInvocationQuery, i, 1);

	vkCmdBeginQuery(cmd, mInvocationQuery, i, 0);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, mQuery, (i * 2) + 0);

	VkBuffer outputDrawDataBuffer = mOutputDrawDataArray->mBuffers[FrameIndex];
	vkCmdFillBuffer(cmd, outputDrawDataBuffer, 0, VK_WHOLE_SIZE, 0);
	VkBufferMemoryBarrier drawBufferBarrier = vk::Gfx_BufferMemoryBarrier(VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, outputDrawDataBuffer);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &drawBufferBarrier, 0, nullptr);
	VkBufferMemoryBarrier computeVertexShaderBarrier[2] = {
		vk::Gfx_BufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, mOutputGeometryDataArray->mBuffers[FrameIndex]),
		vk::Gfx_BufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, mOutputDrawDataArray->mBuffers[FrameIndex])
	};
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 2, computeVertexShaderBarrier, 0, nullptr);

	VkDescriptorSet computeSets[1] = { mSet[i] };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mFrustrum);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mFrustrumLayout, 0, 1, computeSets, 0, nullptr);
	int drawCount = mECS->GetDrawCount();
	int instanceCount = mECS->GetInstanceCount();

	vkCmdDispatch(cmd, (instanceCount + (KernalSizeX - 1)) / KernalSizeX, (drawCount + (KernalSizeY - 1)) / KernalSizeY, 1);

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, mQuery, (i * 2) + 1);
	vkCmdEndQuery(cmd, mInvocationQuery, i);

	vkEndCommandBuffer(cmd);
}

