#pragma once
#include "Scene.hpp"
#include "FrustrumCullPass.hpp"
#include "../Camera.hpp"
#include "../ecs/EntityController.hpp"
#include <backend/VkGraphicsCard.hpp>
#include <memory/Texture2.hpp>
#include <memory/Buffer2.hpp>
#include <material_system/Material.hpp>

extern vk::VkContext gContext;

namespace Application {

	class GeometryPass : public Scene {
	public:
		GeometryPass(IBuffer2 verticesSSBO, IBuffer2 indicesSSBO, MaterialConfiguration& geoConfig, Framebuffer& fbo, FrustumCullPass* cullPass, Camera* camera, EntityController* ecs);
		~GeometryPass();
		void Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		VkCommandBuffer Frame(uint32_t FrameIndex);
		void SetWireframeMode(bool mode);
	private:
		void RecordCommands(uint32_t FrameIndex);
		IBuffer2 mIndicsSSBO;
		FrustumCullPass* mCullPass;
	private:
		Camera* mCamera;
		EntityController* mECS;
	private:
		VkSampler mSampler;
		ITexture2 mWoodTex, mStatueTex;
		Material* mGeoMaterial;
		Framebuffer mFBO;
		friend class SkyboxPass;
	};

}