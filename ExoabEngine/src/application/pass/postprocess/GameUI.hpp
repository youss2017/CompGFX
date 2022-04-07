#pragma once
#include "../Pass.hpp"

namespace Application {

	class GameUI : public Pass {

	public:
		GameUI();
		~GameUI();

		void ReloadShaders();
		VkCommandBuffer Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);

	protected:
		void RecordCommands(uint32_t FrameIndex);

	};

}