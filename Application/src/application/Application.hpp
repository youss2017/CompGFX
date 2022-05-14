#pragma once
#include <graphics/Graphics.hpp>
#include "Layer.hpp"
#include "UI.hpp"
#include "Camera.hpp"
#include "ecs/EntityController.hpp"
#include <pipeline/PipelineCS.hpp>
#include <Logger.hpp>
#include <vector>
#include <glfw/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <backend/VkGraphicsCard.hpp>
#include "pass/FrustrumCullPass.hpp"
#include "pass/GeometryPass.hpp"
#include "pass/SkyboxPass.hpp"
#include "pass/ShadowPass.hpp"
#include "pass/DebugPass.hpp"
#include "pass/postprocess/BloomPass.hpp"
#include "pass/postprocess/GameUI.hpp"
#include "perlin_noise.hpp"
#include <audio/Audio.hpp>
#include "assets/geometry.cfg"
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include "Globals.hpp"
#include "OpenSaveDialog.hpp"
#include "StringUtils.hpp"
#include "minimap.hpp"
#include "../physics/PhysicsEngine.hpp"
#include "CameraControl.hpp"

namespace Application {

	class Application : public Layer {
	public:
		bool OnInitalize();
		void OnFrame();
		void OnDestroy();
	private:
		void CreateTerrain();
	private:
		IGraphics3D mGfx;
		vk::GraphicsSwapchain mSwapchain;
		IBuffer2 mVerticesSSBO, mIndicesBuffer;
		
		ShaderTypes::Light mLight;
		ecs::EntityController* mECS;
		FrustumCullPass* mCullPass;
		GeometryPass* mGeoPass;
		SkyboxPass* mSkybox;
		ShadowPass* mShadow;
		DebugPass* mDebugPass;
		BloomPass* mBloom;
		GameUI* mUI;
		VkSemaphore mShadowPassSemaphore[gFrameOverlapCount];
		VkSemaphore mBloomPassSemaphore[gFrameOverlapCount];
		Terrain* mT0 = nullptr;
		const uint32_t mInstanceCount = 1000;
		ITexture2 mMinimap;
		ITexture2 mNoiseMapTexture;
		ITexture2 mNoiseMapTexture1;
		ITexture2 mNoiseMapTexture2;
		VkSampler mImGuiSampler;

		Ph::PhysicsEngine* mEngine;
		std::vector<Ph::PhysicsEntity*> vEnts;
		CameraControl* mCamera;
	};

}