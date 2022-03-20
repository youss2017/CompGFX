#include "PipelineCS.hpp"
#include <backend/VkGraphicsCard.hpp>

VkPipeline Pipeline_CreateCompute(GraphicsContext _context, Shader* computeShader, VkPipelineLayout layout, VkPipelineCreateFlags flags)
{
	vk::VkContext context = ToVKContext(_context);
	VkDevice device = context->defaultDevice;

	VkComputePipelineCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = flags;
    createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage.pNext = nullptr;
    createInfo.stage.flags = 0;
    createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    createInfo.stage.module = computeShader->GetShader();
    createInfo.stage.pName = computeShader->GetEntryPoint().c_str();
    VkSpecializationInfo info = computeShader->GetSpecializationInfo();
    createInfo.stage.pSpecializationInfo = &info;
    createInfo.layout = layout;
    createInfo.basePipelineHandle = nullptr;
    createInfo.basePipelineIndex = 0;

    VkPipeline compute;
    vkcheck(vkCreateComputePipelines(device, nullptr, 1, &createInfo, nullptr, &compute));

	return compute;
}
