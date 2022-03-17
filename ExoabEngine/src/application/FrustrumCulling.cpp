#include "FrustrumCulling.hpp"

extern vk::VkContext gContext;

namespace Application {
	FrustrumCompute CreateFrustrumCompute(IBuffer2* inputGeometryDataArray, IBuffer2* inputDrawDataArray)
	{
		std::vector<ShaderBinding> computeBindings(4);
		computeBindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
		computeBindings[0].m_bindingID = 0;
		computeBindings[0].m_preinitalized = true;
		computeBindings[0].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
		computeBindings[0].m_ssbo = inputGeometryDataArray;

		computeBindings[1].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
		computeBindings[1].m_bindingID = 1;
		computeBindings[1].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
		computeBindings[1].m_size = inputGeometryDataArray[0]->size;

		computeBindings[2].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
		computeBindings[2].m_bindingID = 2;
		computeBindings[2].m_preinitalized = true;
		computeBindings[2].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
		computeBindings[2].m_ssbo = inputDrawDataArray;

		computeBindings[3].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
		computeBindings[3].m_bindingID = 3;
		computeBindings[3].m_hostvisible = true;
		computeBindings[3].m_useclientbuffer = false;
		computeBindings[3].m_preinitalized = false;
		computeBindings[3].m_additional_buffer_flags = BufferType(BUFFER_TYPE_INDIRECT | BUFER_TYPE_TRANSFER_SRC);
		computeBindings[3].m_shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
		computeBindings[3].m_size = inputDrawDataArray[0]->size;

		std::vector<VkDescriptorPoolSize> poolSize;
		ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSize, &computeBindings);
		FrustrumCompute compute;
		compute.m_pool = vk::Gfx_CreateDescriptorPool(gContext, 1 * gFrameOverlapCount, poolSize);
		compute.m_set0 = ShaderBinding_Create(gContext, compute.m_pool, 0, &computeBindings);
		
		compute.m_layout = ShaderBinding_CreatePipelineLayout(gContext, { compute.m_set0 }, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FrustrumPlanes)} });
		Shader computeShader = Shader(gContext, "assets/shaders/FrustrumCulling.comp");
		compute.m_pipeline = Pipeline_CreateCompute(gContext, &computeShader, compute.m_layout, 0);

		compute.m_outputGeometryDataArray = compute.m_set0->m_bindings[1].m_ssbo;
		compute.m_outputDrawDataArray = compute.m_set0->m_bindings[3].m_ssbo;

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