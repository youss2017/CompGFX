#pragma once
#include <vulkan/vulkan.h>
#include <shaders/Shader.hpp>

VkPipeline PipelineState_CreateCompute(Shader* computeShader, VkPipelineLayout layout, VkPipelineCreateFlags flags);
