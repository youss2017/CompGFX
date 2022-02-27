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
			vec3 bounding_sphere_center;
			float bounding_sphere_radius;
			mat4 m_Model;
			mat4 m_NormalModel;
		};

		struct DrawData
		{
			VkDrawIndexedIndirectCommand command;
			uint ObjectDataIndex;
			uint TexIndex;
		};

		struct TerrainTransform
		{
			mat4 u_View;
			mat4 u_Model;
			mat4 u_NormalModel;
			mat4 u_Projection;
		};

	};

}