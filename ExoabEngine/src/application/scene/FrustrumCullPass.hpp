#pragma once
#include "Scene.hpp"
#include "../Camera.hpp"
#include "../ecs/EntityController.hpp"
#include <memory/Buffer2.hpp>
#include <shaders/ShaderBinding.hpp>

namespace Application {

	class FrustumCullPass : public Scene {

	public:

		// This camera will be used as a pointer, make sure to allocate camera on heap
		FrustumCullPass(EntityController *ecs, Camera* camera);
		~FrustumCullPass();
		void Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		VkCommandBuffer Frame(uint32_t FrameIndex);

	private:
		Camera* mCamera;
		EntityController* mECS;
	private:
		VkDescriptorPool mPool;
		ShaderSet mSet;
		IBuffer2* mOutputGeometryDataArray;
		IBuffer2* mOutputDrawDataArray;
		VkPipelineLayout mFrustrumLayout;
		VkPipeline mFrustrum;
		void* mFrustrumPlanesMapped[gFrameOverlapCount];
		friend class GeometryPass;
	};


}