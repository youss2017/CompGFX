#include "IScene.hpp"
using namespace std;
using namespace vk;

egx::IScene::IScene(const DeviceCtx& ctx) : m_Ctx(ctx), m_CurrentFrame(&ctx->CurrentFrame)
{
	for (size_t i = 0; i < ctx->FramesInFlight; i++) {
		auto pool = ctx->Device.createCommandPool({});
		vk::CommandBuffer cmd = ctx->Device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(pool, vk::CommandBufferLevel::ePrimary, 1))[0];
		m_CommandPools.push_back(pool);
		m_CommandBuffers.push_back(cmd);
		m_FrameFence.push_back(ctx->Device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));
	}
}

void egx::IScene::Process()
{
	int frame = *m_CurrentFrame;
	Fence fence = m_FrameFence[frame];
	m_Ctx->Device.waitForFences(fence, true, numeric_limits<uint64_t>::max());
	m_Ctx->Device.resetFences(fence);
	m_Ctx->Device.resetCommandPool(m_CommandPools[frame]);

 	CommandBuffer cmd = m_CommandBuffers[frame];
	cmd.begin(vk::CommandBufferBeginInfo());

	if (m_RT) {
		m_RT->Begin(cmd);
		m_RT->BeginDearImguiFrame();
	}

	for (auto& stage : m_Stages) {
		stage->Process(cmd);
	}

	if (m_RT) {
		m_RT->EndDearImguiFrame(cmd);
		m_RT->End(cmd);
	}
	cmd.end();

	SubmitInfo submit;
	submit.setCommandBuffers(cmd);
	m_Ctx->Queue.submit(submit, fence);
}
