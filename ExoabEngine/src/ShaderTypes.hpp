#pragma once
#define SHADER_STD140_ALIGN __declspec(align(16))
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

namespace ShaderTypes {

	using namespace glm;
	extern "C" {

		struct SceneData
		{
			SHADER_STD140_ALIGN mat4 m_View;
			SHADER_STD140_ALIGN mat4 m_Projection;
		};

		struct ObjectData
		{
			SHADER_STD140_ALIGN mat4 m_Model;
			SHADER_STD140_ALIGN mat4 m_NormalModel;
		};

		struct DrawData
		{
			VkDrawIndexedIndirectCommand command;
			uint SceneDataIndex;
			SHADER_STD140_ALIGN uint ObjectDataIndex;
		};

	};

}