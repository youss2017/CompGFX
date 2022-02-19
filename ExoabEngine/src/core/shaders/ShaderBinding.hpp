#pragma once
#include <backend/VkGraphicsCard.hpp>
#include <memory/Buffer2.hpp>

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
	uint32_t m_array_size;
	uint32_t m_size;
	VkFormat m_format;
	union {
		IBuffer2 m_buffer;
		IBuffer2 m_dynamic_buffer;
		IBuffer2 m_ssbo;
		IBuffer2 m_dynamic_ssbo;
		// TODO: Texture
	};
};

// Set pHandle to 
ShaderBinding ShaderBinding_Initalize(ShaderBindingFlags type, uint32_t array_size, uint32_t size, void* pHandle, VkFormat format = VK_FORMAT_UNDEFINED);