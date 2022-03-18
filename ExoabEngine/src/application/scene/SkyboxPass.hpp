#pragma once
#include "Scene.hpp"
#include "../Camera.hpp"
#include <string>
#include <memory/CubeMap.hpp>
#include <pipeline/Pipeline.hpp>
#include <shaders/ShaderBinding.hpp>
#include "GeometryPass.hpp"

namespace Application {

	class SkyboxPass : public Scene {

	public:
		SkyboxPass(const std::string& environmentMapPath, GeometryPass* geoPass, Camera* camera);
		~SkyboxPass();

		void Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		VkCommandBuffer Frame(uint32_t FrameIndex);

	private:
		Camera* mCamera;
		GeometryPass* mGeoPass;
		VkSampler mSampler;
		ITexture2 mCubeMap;
		VkPipelineLayout mLayout;
		IPipelineState mState;
		VkDescriptorPool mPool;
		ShaderSet mSet;
	};

}
