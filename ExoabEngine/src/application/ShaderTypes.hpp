#pragma once
#define SHADER_STD140_ALIGN __declspec(align(16))
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#define MAX_BONES 100

namespace ShaderTypes {

	using namespace glm;
	extern "C" {

		struct GlobalData {
			float u_DeltaTime;
			float u_TimeFromStart;
			mat4 u_View;
			mat4 u_Projection;
			mat4 u_ProjView;
		};

		struct InstanceData {
			uint mTextureID[4];
			mat4 mModel;
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
			mat4 finalBoneTransformations[MAX_BONES];
		};

	};

}