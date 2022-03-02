#pragma once
#include "MaterialConfiguration.hpp"
#include "FramebufferReserve.hpp"
#include "../Graphics.hpp"
#include <shaders/ShaderBinding.hpp>
#include <map>
#include <unordered_map>
struct Material
{
	vk::VkContext m_context;
	int width, height;
	Shader* m_vertex_shader;
	Shader* m_fragment_shader;
	ShaderReflection m_vertex_reflection;
	ShaderReflection m_fragment_reflection;
	PipelineVertexInputDescription m_input_description;
	VkDescriptorPool m_pool;
	// Access by setID
	std::map<uint32_t, ShaderSet> m_sets;
	VkPipelineLayout m_layout;
	IPipelineState m_pipeline_state;
};

struct MaterialSetDescription
{
	uint32_t m_setID;
	std::vector<ShaderBinding>* m_binding_ptr;
};

// setBindings must be from lower setID to higher setID
Material* Material_Create(GraphicsContext context, int width, int height, MaterialConfiguration* configuration,
	PipelineVertexInputDescription* vertexInputDescription, const std::vector<MaterialSetDescription>& setBindings, std::vector<VkPushConstantRange> pushblocks);
// setBindings must be from lower setID to higher setID
Material* Material_Create(GraphicsContext context, int width, int height, MaterialConfiguration* configuration,
	PipelineVertexInputDescription* vertexInputDescription, const std::string& absolute_vertex_path, const std::string& absolute_fragment_path, 
	const std::vector<MaterialSetDescription>& setBindings, std::vector<VkPushConstantRange> pushblocks);
void Material_Destory(Material* material);

void Material_RecreatePipeline(Material* material, PipelineSpecification specification);
