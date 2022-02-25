#pragma once
#include "MaterialConfiguration.hpp"
#include "FramebufferReserve.hpp"
#include "../Graphics.hpp"
#include <shaders/ShaderBinding.hpp>
#include <map>
#include <unordered_map>

struct MaterialFramebuffer
{
	MaterialConfiguration m_configuration;
	IFramebuffer m_framebuffer;
	std::vector<ITexture2> m_textures;
	std::vector<VkFramebuffer> m_framebuffers;
};

typedef MaterialFramebuffer* IMaterialFramebuffer;

// Pipeline Shaders are only needed for creation of other material objects
// After their created you can destroy pipeline shaders
IMaterialFramebuffer Material_CreateFramebuffer(GraphicsContext context, MaterialConfiguration* configuration, ConfigurationSettings settings, IFramebufferStateManagement state_managment, FramebufferReserve* reserve);
void Material_DestroyFramebuffer(IMaterialFramebuffer framebuffer);
IPipelineState Material_CreatePipelineState(GraphicsContext context, MaterialConfiguration* configuration, IMaterialPipelineLayout pipeline_layout, IFramebufferStateManagement state_managment);

void Material_DestroyPipelineShaders(IPipelineShaders pipeline_shaders);
void Material_DestroyPipelineLayout(IMaterialPipelineLayout pipeline_layout);

struct Material
{
	vk::VkContext m_context;
	Shader* m_vertex_shader;
	Shader* m_fragment_shader;
	ShaderReflection m_vertex_reflection;
	ShaderReflection m_fragment_reflection;
	PipelineVertexInputDescription m_input_description;
	VkDescriptorPool m_pool;
	// Access by setID
	std::map<uint32_t, ShaderSet> m_sets;
	VkPipelineLayout m_layout;
	IFramebufferStateManagement m_state_managment;
	IPipelineState m_pipeline_state;
};

IFramebufferStateManagement Material_CreateFramebufferStateManagment(GraphicsContext context, MaterialConfiguration* configuration, FramebufferReserve* reserve);
Material* Material_Create(GraphicsContext context, MaterialConfiguration* configuration, IFramebufferStateManagement framebufferStateManagment, std::vector<ShaderBinding> bindings, std::vector<VkPushConstantRange> pushblocks);
Material* Material_Create(GraphicsContext context, MaterialConfiguration* configuration, IFramebufferStateManagement framebufferStateManagment, const std::string& absolute_vertex_path, const std::string& absolute_fragment_path, std::vector<ShaderBinding> bindings, std::vector<VkPushConstantRange> pushblocks);
void Material_Destory(Material* material);
