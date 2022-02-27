#pragma once
#include <vulkan/vulkan.h>
#include <shaders/Shader.hpp>

VkPipeline Pipeline_CreateCompute(GraphicsContext context, Shader* computeShader, VkPipelineLayout layout, VkPipelineCreateFlags flags);
