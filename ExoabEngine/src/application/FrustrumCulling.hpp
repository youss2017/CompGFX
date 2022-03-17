#pragma once
#include <pipeline/PipelineCS.hpp>
#include <shaders/ShaderBinding.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace Application {

	struct FrustrumPlanes {
		int u_MaxGeometry;
		glm::vec4 u_CullPlanes[5];
	};

	struct FrustrumCompute {
		VkPipelineLayout m_layout;
		VkDescriptorPool m_pool;
		ShaderSet m_set0;
		IBuffer2* m_outputGeometryDataArray;
		IBuffer2* m_outputDrawDataArray;
		VkPipeline m_pipeline;
	};

	FrustrumCompute CreateFrustrumCompute(IBuffer2* inputGeometryDataArray, IBuffer2* inputDrawDataArray);
	void DestroyFrusturumCompute(FrustrumCompute& compute);

}
