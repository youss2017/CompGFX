#include "FrustrumCulling.hpp"

extern vk::VkContext gContext;

namespace Application {
	FrustrumCompute CreateFrustrumCompute(IBuffer2* objectDataArray, IBuffer2* inputDrawArray, IBuffer2* ouptutDrawArray)
	{
		std::vector<ShaderBinding> computeBindings(4);
		computeBindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
		computeBindings[0].m_bindingID = 0;
		computeBindings[0].m_hostvisible = true;
		computeBindings[0].m_useclientbuffer = false;
		computeBindings[0].m_preinitalized = true;
		computeBindings[0].m_additional_buffer_flags = (BufferType)0;
		computeBindings[0].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
		computeBindings[0].m_ssbo = objectDataArray;

		computeBindings[1].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
		computeBindings[1].m_bindingID = 1;
		computeBindings[1].m_hostvisible = true;
		computeBindings[1].m_useclientbuffer = false;
		computeBindings[1].m_preinitalized = true;
		computeBindings[1].m_additional_buffer_flags = (BufferType)0;
		computeBindings[1].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
		computeBindings[1].m_ssbo = inputDrawArray;

		computeBindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
		computeBindings[2].m_bindingID = 2;
		computeBindings[2].m_hostvisible = false;
		computeBindings[2].m_useclientbuffer = false;
		computeBindings[2].m_preinitalized = true;
		computeBindings[2].m_additional_buffer_flags = (BufferType)0;
		computeBindings[2].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
		computeBindings[2].m_ssbo = ouptutDrawArray;

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
		FrustrumCompute compute;
		compute.m_pool = vk::Gfx_CreateDescriptorPool(gContext, 1 * gFrameOverlapCount, poolSize);
		compute.m_set0 = ShaderBinding_Create(gContext, compute.m_pool, 0, &computeBindings);
		
		compute.m_layout = ShaderBinding_CreatePipelineLayout(gContext, { compute.m_set0 }, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FrustrumPlanes)} });
		Shader computeShader = Shader(gContext, "assets/shaders/FrustrumCulling.comp");
		compute.m_pipeline = Pipeline_CreateCompute(gContext, &computeShader, compute.m_layout, 0);

		return compute;
	}

	void DestroyFrusturumCompute(FrustrumCompute& compute)
	{
		VkDevice device = gContext->defaultDevice;
		vkDestroyPipelineLayout(device, compute.m_layout, nullptr);
		vkDestroyDescriptorPool(device, compute.m_pool, nullptr);
		ShaderBinding_DestroySets(gContext, { compute.m_set0 });
		vkDestroyPipeline(device, compute.m_pipeline, nullptr);
	}

}