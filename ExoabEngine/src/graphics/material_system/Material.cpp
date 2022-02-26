#include "Material.hpp"
#include "../../core/backend/VkGraphicsCard.hpp"
#include "../../core/shaders/Shader.hpp"
#include "../../core/shaders/ShaderReflection.hpp"
#include "../../utils/defines.h"
#include <cassert>
#include <utility>
#include <algorithm>

constexpr const char *ASSESTS_SHADER_ROOT_PATH = "assets/materials/shaders/";

IFramebufferStateManagement Material_CreateFramebufferStateManagment(GraphicsContext context, MaterialConfiguration *configuration, FramebufferReserve *reserve)
{
	// 2) Create FramebufferStateManagment
	std::vector<FramebufferAttachment> attachments;
	TextureSamples samples = TextureSamples::MSAA_1;
	for (uint32_t i = 0; i < configuration->m_attachments.size(); i++)
	{
		MaterialAttachment &att = configuration->m_attachments[i];
		TextureFormat format = att.format;
		if (att.m_reserve_id != UINT32_MAX)
		{
			assert(att.m_reserve_id < reserve->m_attachments.size());
			format = reserve->m_attachments[att.m_reserve_id]->m_specification.m_Format;
		}
		FramebufferAttachment attachment = FramebufferAttachment::InitalizeNoBlend(
			format,
			att.m_loadop,
			att.m_storeop,
			att.m_initialLayout,
			att.m_finialLayout,
			att.m_imageLayout,
			att.m_clearvalue);
		assert(att.blend_state_id < configuration->m_blend_settings.size() && "Invalid blend state id! Outside range of std::vector");
		MaterialBlendStateSettings &blend_settings = configuration->m_blend_settings[att.blend_state_id];
		attachment.InitalizeBlend(
			blend_settings.blend_enable,
			blend_settings.src_color_blend_factor,
			blend_settings.dst_color_blend_factor,
			blend_settings.color_blend_op,
			blend_settings.src_alpha_blend_factor,
			blend_settings.dst_alpha_blend_factor,
			blend_settings.alpha_blend_op,
			blend_settings.ColorWriteMask_r,
			blend_settings.ColorWriteMask_g,
			blend_settings.ColorWriteMask_b,
			blend_settings.ColorWriteMask_a);
		attachments.push_back(attachment);
	}
	return FramebufferStateManagement_Create(context, attachments, samples, 0.0f, 0.0f, 0.0f, 0.0f);
}

IMaterialFramebuffer Material_CreateFramebuffer(GraphicsContext context, MaterialConfiguration *configuration, ConfigurationSettings settings, IFramebufferStateManagement state_managment, FramebufferReserve *reserve)
{
	IMaterialFramebuffer framebuffer = new MaterialFramebuffer();
	framebuffer->m_configuration = *configuration;
	auto vcont = ToVKContext(context);
	int width = configuration->m_framebuffer_width == UINT32_MAX ? settings.ResolutionWidth : configuration->m_framebuffer_width;
	int height = configuration->m_framebuffer_height == UINT32_MAX ? settings.ResolutionHeight : configuration->m_framebuffer_height;
	// 3) Create Framebuffer
	// Wish it was that simple
	// m_framebuffer = Framebuffer::Create(context, 0, 0, &m_statemanagment);
	// [WARNING]: Since textures could contain global textures (from the reserve) we cannot call the default Destroy() from the framebuffer
	// to actually destroy framebuffer, there must be a custom implementation!
	// std::vector<GPUTexture2D> textures;
	for (uint32_t i = 0; i < configuration->m_attachments.size(); i++)
	{
		MaterialAttachment &att = configuration->m_attachments[i];
		if (att.m_reserve_id == UINT32_MAX)
		{
			// no reserve id, create new texture attachment
			Texture2DSpecification specification;
			specification.m_Width = width;
			specification.m_Height = height;
			specification.m_TextureUsage = (att.format == TextureFormat::D32F) ? TextureUsage::DEPTH_ATTACHMENT : TextureUsage::COLOR_ATTACHMENT;
			specification.m_Samples = TextureSamples::MSAA_1;
			specification.m_Format = att.format;
			specification.m_GenerateMipMapLevels = false;
			specification.m_CreatePerFrame = true;
			specification.m_LazilyAllocate = false;
			framebuffer->m_textures.push_back(Texture2_Create(context, specification));
		}
		else
		{
			// use the reserve id!
			framebuffer->m_textures.push_back(reserve->m_attachments[att.m_reserve_id]);
		}
	}

	for (uint32_t i = 0; i < gFrameOverlapCount; i++)
	{
		// Get Attachments
		VkImageView *pAttachments = (VkImageView *)stack_allocate(sizeof(VkImageView) * framebuffer->m_textures.size());
		for (int j = 0; j < framebuffer->m_textures.size(); j++)
		{
			std::vector<VkImageView> &allviews = framebuffer->m_textures[j]->m_vk_views_per_frame;
			pAttachments[j] = allviews[i];
		}
		VkFramebufferCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.renderPass = (VkRenderPass)state_managment->m_renderpass;
		createInfo.attachmentCount = framebuffer->m_textures.size();
		createInfo.pAttachments = pAttachments;
		createInfo.width = width;
		createInfo.height = height;
		createInfo.layers = 1;
		VkFramebuffer fbo;
		vkcheck(vkCreateFramebuffer(vcont->defaultDevice, &createInfo, vcont->m_allocation_callback, &fbo));
		framebuffer->m_framebuffers.push_back(fbo);
		stack_free(pAttachments);
	}
	framebuffer->m_framebuffer = new Framebuffer();
	framebuffer->m_framebuffer->m_ApiType = 0;
	framebuffer->m_framebuffer->m_context = context;
	framebuffer->m_framebuffer->m_width = width;
	framebuffer->m_framebuffer->m_height = height;
	framebuffer->m_framebuffer->m_attachments = framebuffer->m_textures;
	framebuffer->m_framebuffer->m_framebuffers = framebuffer->m_framebuffers;
	framebuffer->m_framebuffer->m_framebuffer = 0;
	framebuffer->m_framebuffer->m_viewport_x = 0;
	framebuffer->m_framebuffer->m_viewport_y = 0;
	framebuffer->m_framebuffer->m_viewport_width = width;
	framebuffer->m_framebuffer->m_viewport_height = height;
	framebuffer->m_framebuffer->m_scissor_x = 0;
	framebuffer->m_framebuffer->m_scissor_y = 0;
	framebuffer->m_framebuffer->m_scissor_width = width;
	framebuffer->m_framebuffer->m_scissor_height = height;

	return framebuffer;
}

void Material_DestroyFramebuffer(IMaterialFramebuffer framebuffer)
{
	vk::VkContext context = ToVKContext(framebuffer->m_framebuffer->m_context);

	for (uint32_t i = 0; i < framebuffer->m_configuration.m_attachments.size(); i++)
		if (framebuffer->m_configuration.m_attachments[i].m_reserve_id == UINT32_MAX)
			Texture2_Destroy(framebuffer->m_textures[i]);

	for (uint32_t i = 0; i < framebuffer->m_framebuffers.size(); i++)
		vkDestroyFramebuffer(context->defaultDevice, framebuffer->m_framebuffers[i], context->m_allocation_callback);

	delete framebuffer->m_framebuffer;
	delete framebuffer;
}

Material* Material_Create(GraphicsContext _context, MaterialConfiguration* configuration, IFramebufferStateManagement framebufferStateManagment, PipelineVertexInputDescription* vertexInputDescription, const std::vector<MaterialSetDescription>& setBindings, std::vector<VkPushConstantRange> pushblocks)
{
	return Material_Create(_context, configuration, framebufferStateManagment, vertexInputDescription, ASSESTS_SHADER_ROOT_PATH + configuration->vertex_shader, ASSESTS_SHADER_ROOT_PATH + configuration->fragment_shader, setBindings, pushblocks);
}

Material* Material_Create(GraphicsContext _context, MaterialConfiguration* configuration, IFramebufferStateManagement framebufferStateManagment, PipelineVertexInputDescription* vertexInputDescription, const std::string& absolute_vertex_path, const std::string& absolute_fragment_path,
	const std::vector<MaterialSetDescription>& setBindings, std::vector<VkPushConstantRange> pushblocks)
{
	vk::VkContext context = ToVKContext(_context);
	Material* material = new Material();
	material->m_context = context;
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
		ShaderBinding_CalculatePoolSizes(gFrameOverlapCount, poolSize, setBinding.m_binding_ptr);
	}
	material->m_pool = vk::Gfx_CreateDescriptorPool(context, setBindings.size() * gFrameOverlapCount, poolSize);
	uint32_t setID = 0;
	std::vector<ShaderSet> sets;
	for (auto& setBinding : setBindings)
	{
		ShaderSet set = ShaderBinding_Create(context, material->m_pool, setID, setBinding.m_binding_ptr);
		material->m_sets.insert(std::make_pair(setID, set));
		sets.push_back(set);
		setID++;
	}
	material->m_layout = ShaderBinding_CreatePipelineLayout(context, sets, pushblocks);
	material->m_state_managment = framebufferStateManagment;

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
	material->m_pipeline_state = PipelineState_Create(context, specification, framebufferStateManagment, material->m_input_description, material->m_layout, material->m_vertex_shader, material->m_fragment_shader);

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
