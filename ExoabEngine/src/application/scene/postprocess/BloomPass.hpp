#pragma once
#include "../Scene.hpp"

namespace Application {

	class BloomPass : public Scene {

	public:
		BloomPass();
		~BloomPass();

		void Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart);
		VkCommandBuffer Frame(uint32_t FrameIndex);
	private:
	};

}