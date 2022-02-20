#pragma once
#include <backend/VkGraphicsCard.hpp>
#include <memory/Buffer2.hpp>
#include <memory/Texture2.hpp>

enum ShaderBindingFlags : int
{
	SHADER_BINDING_BUFFER                               = 0x0000001,
	SHADER_BINDING_BUFFER_DYNAMIC                       = 0x0000010,
	SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT         = 0x0000100,
	SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT_DYNAMIC = 0x0001000,
	SHADER_BINDING_TEXTURE                              = 0x0010000,
};

struct ShaderBinding
{
	ShaderBindingFlags m_type;
	uint32_t m_bindingID;
	bool m_hostvisible;
	bool m_useclientbuffer;
	BufferType m_additional_flags = (BufferType)0;
	VkShaderStageFlags m_shaderStages;
	uint32_t m_array_size;
	uint32_t m_size;
	// Only for textures
	VkFormat m_format;
	VkDescriptorType m_vk_type;
	// Do not touch anything beyond this point
	// these are arrays with size of framecount 
	union {
		IBuffer2 m_client_buffer;
		IBuffer2* m_buffer;
		IBuffer2* m_dynamic_buffer;
		IBuffer2* m_ssbo;
		IBuffer2* m_dynamic_ssbo;
		// TODO: Texture
	};
};

struct _ShaderSet
{
	uint32_t m_setID;
	std::vector<ShaderBinding> m_bindings;
	VkDescriptorSetLayout m_setlayout;
	// Count is frame count, use m_set[frame index]
	VkDescriptorSet* m_set;
};

typedef _ShaderSet* ShaderSet;

ShaderSet ShaderBinding_Create(vk::VkContext context, VkDescriptorPool pool, uint32_t setID, std::vector<ShaderBinding>& bindings);
VkPipelineLayout ShaderBinding_CreatePipelineLayout(vk::VkContext context, std::vector<ShaderSet> sets, std::vector<VkPushConstantRange> ranges);
