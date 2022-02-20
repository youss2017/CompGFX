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
	// The following must be set
	ShaderBindingFlags m_type;
	uint32_t m_bindingID;
	bool m_hostvisible;
	bool m_useclientbuffer;
	BufferType m_additional_buffer_flags = (BufferType)0;
	VkShaderStageFlags m_shaderStages;
	uint32_t m_size;
	// Only for textures
	VkSampler m_sampler = VK_NULL_HANDLE;
	// Array size is the size of m_textures list
	std::vector<ITexture2> m_textures;
	std::vector<VkImageLayout> m_textures_layouts;
	// Do not touch anything beyond this point
	// these are arrays with size of framecount 
	VkDescriptorType m_vk_type;
	union {
		IBuffer2 m_client_buffer;
		IBuffer2* m_buffer;
		IBuffer2* m_dynamic_buffer;
		IBuffer2* m_ssbo;
		IBuffer2* m_dynamic_ssbo;
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

void ShaderBinding_CalculatePoolSizes(uint32_t framecount, std::vector<VkDescriptorPoolSize>& poolSizes, std::vector<ShaderBinding>& bindings);
ShaderSet ShaderBinding_Create(vk::VkContext context, VkDescriptorPool pool, uint32_t setID, std::vector<ShaderBinding>& bindings);
VkPipelineLayout ShaderBinding_CreatePipelineLayout(vk::VkContext context, std::vector<ShaderSet> sets, std::vector<VkPushConstantRange> ranges);
