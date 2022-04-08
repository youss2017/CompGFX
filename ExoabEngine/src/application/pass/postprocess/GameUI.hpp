#pragma once
#include "../Pass.hpp"
#include "shaders/ShaderConnector.hpp"
#include "shaders/Shader.hpp"
#include "pipeline/Pipeline.hpp"
#include "memory/Texture2.hpp"
#include "shaders/ShaderConnector.hpp"
#include "../../../physics/PhysicsCore.hpp"

namespace Application {

	struct UIVertex {
		glm::vec2 inPosition;
		glm::vec2 inTexCoord;
	};

	class GameUI : public Pass {

	public:
		GameUI(Framebuffer& targetFBO, int colorAttachmentIndex, ITexture2 minimap);
		~GameUI();
		void SetCursorPosition(const Ph::Ray& ray);

		void ReloadShaders();
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		inline void SetMinimap(ITexture2 minimap) { mMinimap = minimap; }

	protected:
		void RecordCommands(uint32_t FrameIndex);

	private:
		ITexture2 mMinimap;
		ITexture2 mCursor;
		VkDescriptorPool mPool;
		std::vector<DescriptorSet> mTextureSets;
		UIVertex *mVertices;
		Framebuffer mFBO;
		VkPipelineLayout mLayout;
		IPipelineState mState;
	};

}