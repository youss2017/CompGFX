#pragma once
#include <backend/VkGraphicsCard.hpp>
#include <memory/Buffer2.hpp>
#include <memory/Texture2.hpp>
#include <memory/CubeMap.hpp>

enum ShaderBindingFlags : int
{
	SHADER_BINDING_UNIFORM_BUFFER                       = 0x00000001,
	SHADER_BINDING_UNIFORM_BUFFER_DYNAMIC               = 0x00000010,
	SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT         = 0x00000100,
	SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT_DYNAMIC = 0x00001000,
	SHADER_BINDING_COMBINED_TEXTURE_SAMPLER             = 0x00010000,
	SHADER_BINDING_TEXTURE                              = 0x00100000,
	SHADER_BINDING_SAMPLER                              = 0x01000000,
	SHADER_BINDING_STORAGE_IMAGE						= 0x10000000,
};

struct ShaderBinding
{
	// The following must be set
	ShaderBindingFlags m_type;
	uint32_t m_bindingID;
	bool m_hostvisible;
	bool m_useclientbuffer;
	bool m_preinitalized = false;
	BufferType m_additional_buffer_flags = (BufferType)0;
	VkShaderStageFlags m_shaderStages;
	uint32_t m_size;
	// Only for textures and sperate samplers
	// Array size is the size of m_textures list
	std::vector<VkSampler> m_sampler;
	std::vector<ITexture2> m_textures;
	std::vector<VkImageLayout> m_textures_layouts;
	bool mUseViewPerMip = false;
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

	inline VkBuffer GetBuffer(uint32_t bindingID, uint32_t frameIndex) {
		return m_bindings[bindingID].m_buffer[frameIndex]->mBuffer;
	}
	inline IBuffer2 GetBuffer2(uint32_t bindingID, uint32_t frameIndex) {
		return m_bindings[bindingID].m_buffer[frameIndex];
	}
	inline IBuffer2* GetBuffer2Array(uint32_t bindingID) {
		return m_bindings[bindingID].m_buffer;
	}
};

typedef _ShaderSet* ShaderSet;

void ShaderBinding_CalculatePoolSizes(uint32_t framecount, std::vector<VkDescriptorPoolSize>& poolSizes, const std::vector<ShaderBinding>* bindings);
ShaderSet ShaderBinding_Create(vk::VkContext context, VkDescriptorPool pool, uint32_t setID, std::vector<ShaderBinding>* bindings);
VkPipelineLayout ShaderBinding_CreatePipelineLayout(vk::VkContext context, std::vector<ShaderSet> sets, std::vector<VkPushConstantRange> ranges);
void ShaderBinding_DestroySets(vk::VkContext context, std::vector<ShaderSet> sets);
