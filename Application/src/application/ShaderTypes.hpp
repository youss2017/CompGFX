#pragma once
#define SHADER_STD140_ALIGN __declspec(align(16))
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

namespace ShaderTypes {

	using namespace glm;
	extern "C" {

		struct GlobalData {
			float u_DeltaTime;
			float u_TimeFromStart;
			vec4 u_LightDirection;
			mat4 u_View;
			mat4 u_Projection;
			mat4 u_ProjView;
			mat4 u_LightSpace;
		};

		struct InstanceData {
			float mSpecularStrength;
			uint mTextureIndex;
			mat4 mModel;
			mat3 mNormalModel;
		};

		struct GeometryData
		{
			vec3 bounding_sphere_center;
			float bounding_sphere_radius;
			uint64_t mInstancePtr = 0x0000000000000000ull;
			uint64_t mCulledInstancePtr = 0x0000000000000000ull;;
		};

		struct DrawData
		{
			uint indexCount;
			uint instanceCount;
			uint firstIndex;
			int  vertexOffset;
			uint firstInstance;
			uint GeometryDataIndex;
		};

		struct TerrainTransform
		{
			mat4 u_View;
			mat4 u_Model;
			mat4 u_NormalModel;
			mat4 u_Projection;
		};

		struct ObjectDataBONE {
			mat4 model;
			mat4 view;
			mat4 projection;
		};

		struct Light {
			glm::vec3 u_Position;
			glm::vec3 u_Color;
			float u_Range;
			float u_AmbientStrength;
		};

	};

	inline glm::mat3 CalculateNormalModel(const glm::mat4& model) {
		return glm::mat3(glm::transpose(glm::inverse(model)));
	}

}