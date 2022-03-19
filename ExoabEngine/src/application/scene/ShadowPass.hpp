#pragma once
#include "Scene.hpp"
#include "FrustrumCullPass.hpp"
#include <pipeline/Framebuffer.hpp>
#include <pipeline/Pipeline.hpp>
#include "../ecs/EntityController.hpp"

namespace Application {

	class ShadowPass : public Scene {

	public:
		ShadowPass(IBuffer2 verticesSSBO, IBuffer2 indices, EntityController* ecs, Camera* camera, int size);
		~ShadowPass();

		void Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		VkCommandBuffer Frame(uint32_t FrameIndex);

		Framebuffer mFBO;
	private:
		VkDescriptorPool mPool;
		ShaderSet mSet;
		VkPipelineLayout mLayout;
		IPipelineState mState;
	private:
		Camera* mCamera;
		EntityController* mECS;
		IBuffer2 mVerticesSSBO;
		IBuffer2 mIndices;
		unsigned int mSize;
		friend class GeometryPass;
	};

}
