#pragma once
#include "Scene.hpp"
#include "FrustrumCullPass.hpp"
#include "../Camera.hpp"
#include "../ecs/EntityController.hpp"
#include "../../mesh/Terrain.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <memory/Texture2.hpp>
#include <memory/Buffer2.hpp>
#include <pipeline/Pipeline.hpp>

extern vk::VkContext gContext;

namespace Application {

	class GeometryPass : public Scene {
	public:
		GeometryPass(IBuffer2 verticesSSBO, IBuffer2 indicesSSBO, const Framebuffer& fbo, FrustumCullPass* cullPass, Camera* camera, EntityController* ecs, ITexture2 shadowMap);
		~GeometryPass();
		
		void ReloadShaders();
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		void SetWireframeMode(bool mode);
		void GetStatistics(bool wait, uint32_t frameIndex, double& passTime, uint64_t& vertexInvocations, uint64_t& fragmentInvocations);
		void SetLightDirection(glm::vec3 lightDirection) { mLightDirection = glm::vec4(glm::normalize(lightDirection), 1.0); }
		void SetLightSpace(glm::mat4 lightSpace) { mLightSpace = lightSpace; }
	private:
		void RecordCommands(uint32_t FrameIndex);
		IBuffer2 mIndicsSSBO;
		FrustumCullPass* mCullPass;
	private:
		Camera* mCamera;
		EntityController* mECS;
		glm::vec4 mLightDirection;
		glm::mat4 mLightSpace;
	private:
		bool mWireframeMode = false;
		Terrain mTerrain;
		VkDescriptorPool mPool;
		DescriptorSet mMapSet;
		VkPipelineLayout mMapLayout;
		IPipelineState mMapState;
		IBuffer2 mMapVertices;
		IBuffer2 mMapIndices;
		int mMapIndicesCount;
		VkQueryPool mQuery;
		VkQueryPool mInvocationQuery;
		VkSampler mSampler;
		VkSampler mShadowSampler;
		ITexture2 mWoodTex;
		ITexture2 mStatueTex;
		ITexture2 mNormalMap;
		DescriptorSet mGeoSet0;
		DescriptorSet mGeoSet1;
		VkPipelineLayout mGeoLayout;
		IPipelineState mGeoState;
		Framebuffer mFBO;
		friend class SkyboxPass;
	};

}