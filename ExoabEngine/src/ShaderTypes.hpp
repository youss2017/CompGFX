#pragma once
#define SHADER_STD140_ALIGN __declspec(align(16))
#include <vulkan/vulkan_core.h>

namespace ShaderTypes {

	using namespace glm;
	extern "C" {
		struct Vertex
		{
			vec3 inPosition;
			vec3 inNormal;
			vec2 inTexCoord;
		};

		struct ObjectData
		{
			SHADER_STD140_ALIGN mat4 m_Model;
			SHADER_STD140_ALIGN mat4 m_NormalModel;
			SHADER_STD140_ALIGN mat4 m_View;
			SHADER_STD140_ALIGN mat4 m_Projection;
		};

		struct DrawData
		{
			//VkDrawIndexedIndirectCommand command;
			VkDrawIndirectCommand command;
			SHADER_STD140_ALIGN uint ObjectDataIndex;
		};

	};

}