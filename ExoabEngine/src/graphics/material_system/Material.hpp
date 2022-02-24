#pragma once
#include "MaterialConfiguration.hpp"
#include "FramebufferReserve.hpp"
#include "../Graphics.hpp"
#include <map>
#include <unordered_map>

struct PipelineShaders
{
	Shader *m_vertex_shader;
	Shader *m_fragment_shader;
	ShaderReflection* m_vertex_reflection;
	ShaderReflection* m_fragment_reflection;
};

struct MaterialFramebuffer
{
	MaterialConfiguration m_configuration;
	IFramebuffer m_framebuffer;
	std::vector<ITexture2> m_textures;
	std::vector<VkFramebuffer> m_framebuffers;
};

struct MaterialPipelineLayout
{
	GraphicsContext m_context;
	VkPipelineLayout m_layout;
	PipelineVertexInputDescription input_description;
	PipelineShaders* m_pipeline_shaders;
};

typedef PipelineShaders* IPipelineShaders;
typedef MaterialFramebuffer* IMaterialFramebuffer;
typedef MaterialPipelineLayout* IMaterialPipelineLayout;

// Pipeline Shaders are only needed for creation of other material objects
// After their created you can destroy pipeline shaders
IPipelineShaders Material_CreatePipelineShaders(GraphicsContext context, MaterialConfiguration* configuration);
IPipelineShaders Material_CreatePipelineShaders(GraphicsContext context, const std::string& vertex_shader_path, const std::string& fragment_shader_path);
IFramebufferStateManagement Material_CreateFramebufferStateManagment(GraphicsContext context, MaterialConfiguration* configuration, FramebufferReserve* reserve);
IMaterialFramebuffer Material_CreateFramebuffer(GraphicsContext context, MaterialConfiguration* configuration, ConfigurationSettings settings, IFramebufferStateManagement state_managment, FramebufferReserve* reserve);
IMaterialPipelineLayout Material_CreatePipelineLayout(GraphicsContext context, VkPipelineLayout layout, PipelineVertexInputDescription input_description, IPipelineShaders pipeline_shaders);
IPipelineState Material_CreatePipelineState(GraphicsContext context, MaterialConfiguration* configuration, IMaterialPipelineLayout pipeline_layout, IFramebufferStateManagement state_managment);

void Material_DestroyPipelineShaders(IPipelineShaders pipeline_shaders);
void Material_DestroyFramebuffer(IMaterialFramebuffer framebuffer);
void Material_DestroyPipelineLayout(IMaterialPipelineLayout pipeline_layout);

struct Material
{
	IPipelineShaders m_pipeline_shaders;
	IMaterialPipelineLayout m_pipeline_layout;
	IFramebufferStateManagement m_state_managment;
	IMaterialFramebuffer m_framebuffer;
	IPipelineState m_pipeline_state;

	void Initalize(IPipelineShaders pipeline_shaders, IMaterialPipelineLayout pipeline_layout, IFramebufferStateManagement state_managment, IMaterialFramebuffer framebuffer, IPipelineState pipeline_state)
	{
		m_pipeline_shaders = pipeline_shaders;
		m_pipeline_layout = pipeline_layout;
		m_state_managment = state_managment;
		m_framebuffer = framebuffer;
		m_pipeline_state = pipeline_state;
	}

	void DestoryEverything()
	{
		Material_DestroyPipelineShaders(m_pipeline_shaders);
		Material_DestroyPipelineLayout(m_pipeline_layout);
		FramebufferStateManagment_Destroy(m_state_managment);
		Material_DestroyFramebuffer(m_framebuffer);
		PipelineState_Destroy(m_pipeline_state);
	}

};
