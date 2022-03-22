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
		glm::mat4 u_ProjView;
	};

	class DebugPass : public Scene {
	public:
		DebugPass(const Framebuffer& fbo);
		~DebugPass();

		void SetProjectionView(const glm::mat4& proj, const glm::mat4& view) { mProjView = proj * view; }
		// Call prepare before doing anything else
		void Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		void DrawCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color);

		// Call this when you are done rendering debug objects
		VkCommandBuffer Frame(uint32_t FrameIndex);

	private:
		Framebuffer mFBO;
		glm::mat4 mProjView;
		std::vector<DebugObject> mCubes;
		VkPipelineLayout mLayout;
		IPipelineState mState;
	};

}