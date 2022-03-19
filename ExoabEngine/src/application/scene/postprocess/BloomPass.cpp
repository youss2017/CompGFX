#include "BloomPass.hpp"

Application::BloomPass::BloomPass() : Scene(nullptr, true)
{
}

Application::BloomPass::~BloomPass()
{
}

void Application::BloomPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart)
{
}

VkCommandBuffer Application::BloomPass::Frame(uint32_t FrameIndex)
{
	return VkCommandBuffer();
}
