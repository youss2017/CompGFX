#pragma once
#include <vulkan/vulkan.h>
#include <shaders/Shader.hpp>

VkPipeline GRAPHICS_API PipelineState_CreateCompute(Shader* computeShader, VkPipelineLayout layout, VkPipelineCreateFlags flags);
