#include "BloomPass.hpp"
#include <backend/VkGraphicsCard.hpp>

extern vk::VkContext gContext;

Application::BloomPass::BloomPass(const Framebuffer& fbo) : Scene(gContext->defaultDevice, true) {

}

Application::BloomPass::~BloomPass() {
	Super_Scene_Destroy();
}

void Application::BloomPass::ReloadShaders() {

}

VkCommandBuffer  Application::BloomPass::Prepare(uint32_t FrameIndex, float dTime, float dTimeFromStart) {
	return mCmds[FrameIndex];
}

void Application::BloomPass::RecordCommands(uint32_t FrameIndex) {

}
