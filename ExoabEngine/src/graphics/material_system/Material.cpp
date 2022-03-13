#include "Material.hpp"
#include "../../core/backend/VkGraphicsCard.hpp"
#include "../../core/shaders/Shader.hpp"
#include "../../core/shaders/ShaderReflection.hpp"
#include "../../utils/defines.h"
#include <cassert>
#include <utility>
#include <algorithm>

constexpr const char *ASSESTS_SHADER_ROOT_PATH = "assets/shaders/";

Material* Material_Create(GraphicsContext _context, Framebuffer fbo, MaterialConfiguration* configuration, PipelineVertexInputDescription* vertexInputDescription, const std::vector<MaterialSetDescription>& setBindings, std::vector<VkPushConstantRange> pushblocks)
{
	return Material_Create(_context, fbo, configuration, vertexInputDescription, ASSESTS_SHADER_ROOT_PATH + configuration->vertex_shader, ASSESTS_SHADER_ROOT_PATH + configuration->fragment_shader, setBindings, pushblocks);
}

Material* Material_Create(GraphicsContext _context, Framebuffer fbo, MaterialConfiguration* configuration, PipelineVertexInputDescription* vertexInputDescription, const std::string& absolute_vertex_path, const std::string& absolute_fragment_path,
	const std::vector<MaterialSetDescription>& setBindings, std::vector<VkPushConstantRange> pushblocks)
{
	vk::VkContext context = ToVKContext(_context);
	Material* material = new Material();
	material->m_context = context;
	material->width = fbo.m_width;
	material->height = fbo.m_height;
	// 1) Load Shaders
	material->m_vertex_shader = new Shader(context, absolute_vertex_path.c_str());
	material->m_fragment_shader = new Shader(context, absolute_fragment_path.c_str());
	material->m_vertex_reflection = ShaderReflection(material->m_vertex_shader);
	material->m_fragment_reflection = ShaderReflection(material->m_fragment_shader);

	if (vertexInputDescription)
	{
		material->m_input_description = *vertexInputDescription;
	}

	std::vector<VkDescriptorPoolSize> poolSize;
	for (auto& setBinding : setBindings)
	{
		if (setBinding.m_binding_ptr)
			ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSize, setBinding.m_binding_ptr);
	}
	material->m_pool = vk::Gfx_CreateDescriptorPool(context, setBindings.size() * gFrameOverlapCount, poolSize);
	uint32_t setID = 0;
	std::vector<ShaderSet> sets;
	for (auto& setBinding : setBindings)
	{
		ShaderSet set = nullptr;
		if (setBinding.m_binding_ptr) {
			set = ShaderBinding_Create(context, material->m_pool, setID, setBinding.m_binding_ptr);
		}
		else {
			set = setBinding.m_set;
		}
		material->m_sets.insert(std::make_pair(setBinding.m_setID, set));
		sets.push_back(set);
		setID++;
	}
	material->m_layout = ShaderBinding_CreatePipelineLayout(context, sets, pushblocks);

	PipelineSpecification specification;
	specification.m_CullMode = configuration->cullmode;
	specification.m_DepthEnabled = configuration->DepthTestEnable;
	specification.m_DepthWriteEnable = configuration->DepthWriteEnable;
	specification.m_DepthFunc = configuration->compareop;
	specification.m_PolygonMode = configuration->polygonmode;
	specification.m_FrontFaceCCW = configuration->CCWFrontFace;
	specification.m_Topology = PolygonTopology::TRIANGLE_LIST;
	specification.m_SampleRateShadingEnabled = configuration->SampleShading;
	specification.m_MinSampleShading = Utils::ClampValues(configuration->minSampleShading, 0.0f, 1.0f);
	material->m_pipeline_state = PipelineState_Create(context, specification, material->m_input_description, fbo, material->m_layout, material->m_vertex_shader, material->m_fragment_shader);
	material->m_fbo = fbo;
	return material;
}

void Material_Destory(Material* material)
{
	delete material->m_vertex_shader;
	delete material->m_fragment_shader;
	vkDestroyDescriptorPool(material->m_context->defaultDevice, material->m_pool, nullptr);
	std::vector<ShaderSet> sets;
	for (auto& set : material->m_sets)
		sets.push_back(set.second);
	ShaderBinding_DestroySets(material->m_context, sets);
	vkDestroyPipelineLayout(material->m_context->defaultDevice, material->m_layout, nullptr);
	PipelineState_Destroy(material->m_pipeline_state);
	delete material;
}

void Material_RecreatePipeline(Material* material, PipelineSpecification specification)
{
	PipelineState_Destroy(material->m_pipeline_state);
	material->m_pipeline_state = PipelineState_Create(material->m_context, specification, material->m_input_description, material->m_fbo, material->m_layout, material->m_vertex_shader, material->m_fragment_shader);
}
