#pragma once
#include <vulkan/vulkan.h>
#include <shaders/Shader.hpp>

VkPipeline PipelineState_CreateCompute(GraphicsContext context, Shader* computeShader, VkPipelineLayout layout, VkPipelineCreateFlags flags);
