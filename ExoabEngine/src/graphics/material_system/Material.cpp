#include "Material.hpp"
#include "../../core/backend/VkGraphicsCard.hpp"
#include "../../core/backend/GLGraphicsCard.hpp"
#include "../../core/shaders/Shader.hpp"
#include "../../core/shaders/ShaderReflection.hpp"
#include "../../utils/defines.h"
#include <cassert>
#include <utility>
#include <algorithm>

constexpr const char *ASSESTS_SHADER_ROOT_PATH = "assets/materials/shaders/";

IPipelineShaders Material_CreatePipelineShaders(GraphicsContext context, MaterialConfiguration* configuration)
{
	IPipelineShaders shaders = new PipelineShaders();
	// 1) Load Shaders
	std::string vertex_shader_path = ASSESTS_SHADER_ROOT_PATH + configuration->vertex_shader;
	std::string fragment_shader_path = ASSESTS_SHADER_ROOT_PATH + configuration->fragment_shader;
	shaders->m_vertex_shader = new Shader(context, vertex_shader_path.c_str());
	shaders->m_fragment_shader = new Shader(context, fragment_shader_path.c_str());
	shaders->m_vertex_reflection = new ShaderReflection(shaders->m_vertex_shader);
	shaders->m_fragment_reflection = new ShaderReflection(shaders->m_fragment_shader);
	return shaders;
}

IPipelineShaders Material_CreatePipelineShaders(GraphicsContext context, const std::string& _vertex_shader_path, const std::string& _fragment_shader_path)
{
	IPipelineShaders shaders = new PipelineShaders();
	// 1) Load Shaders
	std::string vertex_shader_path = ASSESTS_SHADER_ROOT_PATH + _vertex_shader_path;
	std::string fragment_shader_path = ASSESTS_SHADER_ROOT_PATH + _fragment_shader_path;
	shaders->m_vertex_shader = new Shader(context, vertex_shader_path.c_str());
	shaders->m_fragment_shader = new Shader(context, fragment_shader_path.c_str());
	shaders->m_vertex_reflection = new ShaderReflection(shaders->m_vertex_shader);
	shaders->m_fragment_reflection = new ShaderReflection(shaders->m_fragment_shader);
	return shaders;
}

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
			GPUTexture2DSpecification specification;
			specification.m_Width = width;
			specification.m_Height = height;
			specification.m_TextureUsage = (att.format == TextureFormat::D32F) ? TextureUsage::DEPTH_ATTACHMENT : TextureUsage::COLOR_ATTACHMENT;
			specification.m_Samples = TextureSamples::MSAA_1;
			specification.m_Format = att.format;
			specification.m_GenerateMipMapLevels = false;
			specification.m_CreatePerFrame = true;
			specification.m_LazilyAllocate = false;
			framebuffer->m_textures.push_back(GPUTexture2D_Create(context, &specification));
		}
		else
		{
			// use the reserve id!
			framebuffer->m_textures.push_back(reserve->m_attachments[att.m_reserve_id]);
		}
	}
	// std::vector<VkFramebuffer> framebuffers;
	for (uint32_t i = 0; i < vcont->FrameInfo->m_FrameCount; i++)
	{
		// Get Attachments
		VkImageView *pAttachments = (VkImageView *)stack_allocate(sizeof(VkImageView) * framebuffer->m_textures.size());
		for (int j = 0; j < framebuffer->m_textures.size(); j++)
		{
			std::vector<VkImageView> &allviews = framebuffer->m_textures[j]->m_views;
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

IMaterialPipelineLayout Material_CreatePipelineLayout(GraphicsContext context, PipelineVertexInputDescription &input_description, IPipelineShaders pipeline_shaders)
{
	IMaterialPipelineLayout pipeline_layout = new MaterialPipelineLayout();
	auto vcont = ToVKContext(context);
	pipeline_layout->m_context = vcont;
	pipeline_layout->m_pipeline_shaders = pipeline_shaders;

	pipeline_layout->m_layout = PipelineLayout_Create(context, input_description, pipeline_shaders->m_vertex_reflection, pipeline_shaders->m_fragment_reflection);
	for (auto &set : pipeline_layout->m_layout->m_set_descs)
	{
		auto &uniform_buffers = set.m_UniformBuffers;
		auto &uniform_textures = set.m_SampledImage;
		std::sort(uniform_buffers.begin(), uniform_buffers.end(), [](UniformBufferDescription &a, UniformBufferDescription &b)
				  { return a.m_Binding < b.m_Binding; });
		std::sort(uniform_textures.begin(), uniform_textures.end(), [](SampledImageDescription &a, SampledImageDescription &b)
				  { return a.m_Binding < b.m_Binding; });
	}
	// 4) Create Descriptor Sets (Per Frame)
	// std::vector<VkDescriptorSet> descriptor_sets;
	if (pipeline_layout->m_layout->m_real_setlayouts.size())
	{
		pipeline_layout->m_pool = vk::Gfx_CreateDescriptorPool(vcont, vcont->FrameInfo->m_FrameCount * pipeline_layout->m_layout->m_real_setlayouts.size(),
															   {
																   {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
																   {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
															   });
		{
			for (uint32_t i = 0; i < vcont->FrameInfo->m_FrameCount; i++)
			{
				VkDescriptorSetAllocateInfo allocInfo;
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.pNext = nullptr;
				allocInfo.descriptorPool = pipeline_layout->m_pool;
				allocInfo.descriptorSetCount = pipeline_layout->m_layout->m_real_setlayouts.size();
				allocInfo.pSetLayouts = pipeline_layout->m_layout->m_real_setlayouts.data();
				VkDescriptorSet *pSets = (VkDescriptorSet *)stack_allocate(sizeof(VkDescriptorSet) * pipeline_layout->m_layout->m_real_setlayouts.size());
				vkcheck(vkAllocateDescriptorSets(vcont->defaultDevice, &allocInfo, pSets));
				for (int j = 0; j < allocInfo.descriptorSetCount; j++)
					pipeline_layout->m_descriptor_sets.push_back(pSets[j]);
				stack_free(pSets);
			}
		}
	}
	return pipeline_layout;
}

IPipelineState Material_CreatePipelineState(GraphicsContext context, MaterialConfiguration *configuration, IMaterialPipelineLayout pipeline_layout, IFramebufferStateManagement state_managment)
{
	// 5) Create Pipeline
	PipelineSpecification specification;
	specification.m_CullMode = configuration->cullmode;
	specification.m_DepthEnabled = configuration->DepthTestEnable;
	specification.m_DepthWriteEnable = configuration->DepthWriteEnable;
	specification.m_DepthFunc = configuration->compareop;
	specification.m_PolygonMode = configuration->polygonmode;
	specification.m_FrontFaceCCW = configuration->CCWFrontFace;	   // Is the front face Counter Clockwise (TRUE) or Clockwise (FALSE)
	specification.m_Topology = PolygonTopology::TRIANGLE_LIST; // TODO: make this more flexable
	specification.m_SampleRateShadingEnabled = configuration->SampleShading;
	specification.m_MinSampleShading = Utils::ClampValues(configuration->minSampleShading, 0.0f, 1.0f);
	return PipelineState_Create(context, specification, state_managment, pipeline_layout->m_layout, pipeline_layout->m_pipeline_shaders->m_vertex_shader, pipeline_layout->m_pipeline_shaders->m_fragment_shader);
}

void Material_DestroyPipelineShaders(IPipelineShaders pipeline_shaders)
{
	delete pipeline_shaders->m_vertex_shader;
	delete pipeline_shaders->m_fragment_shader;
	delete pipeline_shaders->m_vertex_reflection;
	delete pipeline_shaders->m_fragment_reflection;
	delete pipeline_shaders;
}

void Material_DestroyPipelineLayout(IMaterialPipelineLayout pipeline_layout)
{
	vk::VkContext context = ToVKContext(pipeline_layout->m_context);
	if (pipeline_layout->m_pool)
	{
		vkDestroyDescriptorPool(context->defaultDevice, pipeline_layout->m_pool, context->m_allocation_callback);
	}
	PipelineLayout_Destroy(pipeline_layout->m_layout);
}

void Material_DestroyFramebuffer(IMaterialFramebuffer framebuffer)
{
	vk::VkContext context = ToVKContext(framebuffer->m_framebuffer->m_context);

	for (uint32_t i = 0; i < framebuffer->m_configuration.m_attachments.size(); i++)
		if (framebuffer->m_configuration.m_attachments[i].m_reserve_id == UINT32_MAX)
			GPUTexture2D_Destroy(framebuffer->m_textures[i]);

	for (uint32_t i = 0; i < framebuffer->m_framebuffers.size(); i++)
		vkDestroyFramebuffer(context->defaultDevice, framebuffer->m_framebuffers[i], context->m_allocation_callback);

	delete framebuffer->m_framebuffer;
	delete framebuffer;
}

