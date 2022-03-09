#pragma once
#include <pipeline/PipelineCS.hpp>
#include <shaders/ShaderBinding.hpp>
#include <glm/vec4.hpp>

namespace Application {

	struct FrustrumPlanes {
		int u_MaxDraw;
		int padding[3];
		glm::vec4 u_CullPlanes[5];
	};

	struct FrustrumCompute {
		VkPipelineLayout m_layout;
		VkDescriptorPool m_pool;
		ShaderSet m_set0;
		VkPipeline m_pipeline;
	};

	FrustrumCompute CreateFrustrumCompute(IBuffer2* objectDataArray, IBuffer2* inputDrawArray, IBuffer2* ouptutDrawArray);
	void DestroyFrusturumCompute(FrustrumCompute& compute);

}
