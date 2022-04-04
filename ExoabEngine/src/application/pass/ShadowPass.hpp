#pragma once
#include "Pass.hpp"
#include "FrustrumCullPass.hpp"
#include <pipeline/Framebuffer.hpp>
#include <pipeline/Pipeline.hpp>
#include "../ecs/EntityController.hpp"
#include "../../mesh/Terrain.hpp"
#include "../Camera.hpp"
#include <stdint.h>

namespace Application {

	class ShadowPass : public Pass {

	public:
		ShadowPass(IBuffer2 verticesSSBO, IBuffer2 indices, Terrain* terrain, EntityController* ecs, Camera* camera, int size);
		~ShadowPass();

		void ReloadShaders();
		inline void SetShadowLightPosition(glm::vec3 position) { mLightPosition = position; }

		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		void GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime);

		ITexture2 GetDepthAttachment() {
			return mFBO.m_depth_attachment.value().GetAttachment();
		}

		VkImage GetDepthImage(uint32_t frameIndex) {
			return mFBO.m_depth_attachment.value().GetImage(frameIndex);
		}

		VkImageView GetDepthView(uint32_t frameIndex) {
			return mFBO.m_depth_attachment.value().GetView(frameIndex);
		}

		glm::mat4 GetLightSpace();

	private:
		void RecordCommands(uint32_t FrameIndex);

	private:
		VkQueryPool mQuery;
		Framebuffer mFBO;
		VkDescriptorPool mPool;
		DescriptorSet mSet;
		VkPipelineLayout mLayout;
		IPipelineState mState;

		DescriptorSet mTerrainSet;
		VkPipelineLayout mTerrainLayout;
		IPipelineState mTerrainState;

		glm::vec3 mLightPosition;
	private:
		Terrain* mT0;
		Camera* mCamera;
		EntityController* mECS;
		IBuffer2 mVerticesSSBO;
		IBuffer2 mIndices;
		unsigned int mSize;
		friend class GeometryPass;
	};

}
