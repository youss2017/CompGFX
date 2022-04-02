#pragma once
#include "Scene.hpp"
#include <pipeline/Pipeline.hpp>
#include <memory/Buffer2.hpp>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace Application {

	struct DebugObject {
		glm::vec3 inOffset;
		glm::vec3 inScale;
		glm::vec3 inColor;
	};

	struct DebugPushblock {
		glm::mat4 u_ProjView;
		uint64_t u_DebugObjectPtr;
		uint32_t u_DebugInstanceOffset;
	};

	class DebugPass : public Scene {
	public:
		DebugPass(const Framebuffer& fbo);
		~DebugPass();

		void ReloadShaders();
		void SetProjectionView(const glm::mat4& proj, const glm::mat4& view) { mProjView = proj * view; }
		// Call prepare before doing anything else
		void NewFrame() { mCubesCount = 0; mCubesInFrontCount = 0; mDebugObjectIndex = 0; }
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		// RequiredSize refers to the amount of objects that will be rendered so that the buffer can be only allocated once.
		void DrawCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color, bool inFront, uint32_t requiredSize = 0);
		void GetStatistics(bool Wait, uint32_t FrameIndex, double& dTime);

	private:
		void RecordCommands(uint32_t FrameIndex);

	private:
		VkQueryPool mQuery;
		Framebuffer mFBO;
		glm::mat4 mProjView;
		uint32_t mCubesCount;
		uint32_t mCubesInFrontCount;
		VkPipelineLayout mLayout;
		IPipelineState mState;
		IPipelineState mStateInFront;
		uint32_t mDebugObjectCount;
		uint32_t mDebugObjectIndex;
		DebugObject* mDebugBuffer;
		uint64_t mDebugBufferPointer;
	};

}