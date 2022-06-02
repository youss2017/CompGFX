#pragma once
#include "Pass.hpp"
#include "../Camera.hpp"
#include "../ecs/EntityController.hpp"
#include <memory/Buffer2.hpp>
#include <shaders/ShaderConnector.hpp>

namespace Application {

	class FrustumCullPass : public Pass {

	public:

		// This camera will be used as a pointer, make sure to allocate camera on heap
		FrustumCullPass(ecs::EntityController *ecs, Camera* camera, bool enableCulling);
		~FrustumCullPass();

		void ReloadShaders();
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		VkResult GetComputeShaderStatistics(bool Wait, uint32_t FrameIndex, double& duration, uint64_t& invocations);

	private:
		void RecordCommands(uint32_t FrameIndex);

	private:
		Camera* mCamera;
		ecs::EntityController* mECS;
	private:
		VkDescriptorPool mPool;
		DescriptorSet mSet;
		IBuffer2 mOutputGeometryDataArray;
		IBuffer2 mOutputDrawDataArray;
		VkPipelineLayout mFrustrumLayout;
		VkPipeline mFrustrum;
		VkQueryPool mQuery;
		VkQueryPool mInvocationQuery;
		void* mFrustrumPlanesMapped[gFrameOverlapCount];
		bool mEnableCulling;
		friend class GeometryPass;
	};


}