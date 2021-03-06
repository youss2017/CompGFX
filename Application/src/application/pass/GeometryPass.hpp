#pragma once
#include "Pass.hpp"
#include "FrustrumCullPass.hpp"
#include "../Camera.hpp"
#include "../ecs/EntityController.hpp"
#include "../../mesh/Terrain.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <memory/Texture2.hpp>
#include <memory/Buffer2.hpp>
#include <pipeline/Pipeline.hpp>

namespace Application {

	class GeometryPass : public Pass {
	public:
		GeometryPass(uint32_t lightCount, ShaderTypes::Light* lights, IBuffer2 verticesSSBO, IBuffer2 indicesSSBO, Terrain* terrain, int width, int height, FrustumCullPass* cullPass, Camera* camera, Camera* lockedCamera, ecs::EntityController* ecs, ITexture2 shadowMap);
		~GeometryPass();

		// [Warning] This is an expensive operation, since we have to recreate the pipeline
		//void ChangeLights(uint32_t lightCount, Light* lights);
		
		void ReloadShaders();
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		void SetWireframeMode(bool mode);
		void GetStatistics(bool wait, uint32_t frameIndex, double& passTime, uint64_t& vertexInvocations, uint64_t& fragmentInvocations);
		void SetLightDirection(glm::vec3 lightDirection) { mLightDirection = glm::vec4(glm::normalize(lightDirection), 1.0); }
		void SetLightSpace(glm::mat4 lightSpace) { mLightSpace = lightSpace; }
		Framebuffer& GetFramebuffer() { return mFBO; }
		ITexture2 CreateMinimap();

	protected:
		void CullTerrain(const glm::mat4& proj, const glm::mat4& view);
		void RecordCommands(uint32_t FrameIndex);
		IBuffer2 mIndicsSSBO;
		FrustumCullPass* mCullPass;
	private:
		uint32_t mLightCount;
		ShaderTypes::Light* mLights;
		Camera* mCamera;
		Camera* mLockedCamera;
		ecs::EntityController* mECS;
		glm::vec4 mLightDirection;
		glm::mat4 mLightSpace;
	private:
		bool mWireframeMode = false;
		Terrain* mT0;
		VkDescriptorPool mPool;
		DescriptorSet mMapSet;
		VkPipelineLayout mMapLayout;
		IPipelineState mMapState;
		VkQueryPool mQuery;
		VkQueryPool mInvocationQuery;
		VkSampler mShadowSampler;
		ITexture2 mWoodTex;
		ITexture2 mStatueTex;
		ITexture2 mNormalMap;
		ITexture2 mSandTex;
		DescriptorSet mGeoSet0;
		DescriptorSet mGeoSet1;
		VkPipelineLayout mGeoLayout;
		IPipelineState mGeoState;
		Framebuffer mFBO;
		VkDrawIndexedIndirectCommand* mTerrainDrawCommands;
		uint32_t* mTerrainDrawCount;
		friend class SkyboxPass;
	};

}