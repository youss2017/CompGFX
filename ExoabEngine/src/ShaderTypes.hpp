#pragma once
#define SHADER_STD140_ALIGN __declspec(align(16))
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

namespace ShaderTypes {

	using namespace glm;
	extern "C" {

		struct ObjectData
		{
			SHADER_STD140_ALIGN mat4 m_Model;
			SHADER_STD140_ALIGN mat4 m_NormalModel;
			// TODO: Move these to UBOs
			SHADER_STD140_ALIGN mat4 m_View;
			SHADER_STD140_ALIGN mat4 m_Projection;
		};

		struct DrawData
		{
			VkDrawIndexedIndirectCommand command;
			SHADER_STD140_ALIGN uint ObjectDataIndex;
		};

	};

}