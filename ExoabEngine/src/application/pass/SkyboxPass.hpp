#pragma once
#include "Pass.hpp"
#include "../Camera.hpp"
#include <string>
#include <memory/CubeMap.hpp>
#include <pipeline/Pipeline.hpp>
#include <shaders/ShaderConnector.hpp>
#include "GeometryPass.hpp"

namespace Application {

	class SkyboxPass : public Pass {

	public:
		SkyboxPass(const std::string& environmentMapPath, GeometryPass* geoPass, Camera* camera, bool UsingDebugPass);
		~SkyboxPass();

		void ReloadShaders();
		void SetLOD(int lod) { mLod = glm::clamp(lod, 0, (int)mCubeMap->mMipCount); }
		int GetMaxLOD() { return mCubeMap->mMipCount; }

		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);

	private:
		void RecordCommands(uint32_t FrameIndex);

	private:
		bool mUsingDebugPass;
		int mLod = 0;
		Camera* mCamera;
		GeometryPass* mGeoPass;
		ITexture2 mCubeMap;
		VkPipelineLayout mLayout;
		IPipelineState mState;
		VkDescriptorPool mPool;
		DescriptorSet mSet;
	};

}
