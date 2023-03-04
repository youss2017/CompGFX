#include "Scene2.hpp"

using namespace egx::scene;

void egx::scene::Run(const std::initializer_list<IScene*>& scenes)
{
	std::vector<ref<Fence>> fences;
	for (auto& item : scenes)
	{
		fences.push_back(item->GetOrCreateCompletionFence());
	}
	Fence::Wait(fences, UINT64_MAX);
	Fence::Reset(fences);

	for (auto& item : scenes)
	{
		auto currentFrame = item->_CoreInterface->CurrentFrame;
		//assert(item->_SceneComponents.size() > 0 && "You must have at least one scene component.");
		auto currentTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		if (item->_ResizeFramebuffer)
			item->InternalResizeFramebuffer();
		item->SetExecuting();
		item->Update();
		for (auto& lc : item->_SceneLogicComponents) {
			lc->Update(item->_DTime);
		}
		VkCommandBuffer cmd{};
		auto record = [&]()
		{
			cmd = item->_Cmd->GetBuffer();
			item->_Framebuffer->Bind(cmd);
			item->PreRender(cmd);
			double dTime = item->_DTime;
			for (auto& c : item->_SceneComponents)
			{
				if (item->_ReadyComponents.at(size_t(c.get()))) {
					c->Update(dTime);
					c->Run(cmd, dTime);
				}
			}
			item->PostRender(cmd);
			vkCmdEndRenderPass(cmd);
			item->_Cmd->Finalize();
			item->AfterRender();
		};

		if (item->_CmdStaticRecord)
		{
			if (item->_CmdStaticInit[currentFrame]) {
				double dTime = item->_DTime;
				for (auto& c : item->_SceneComponents)
				{
					if (item->_ReadyComponents.at(size_t(c.get()))) {
						c->Update(dTime);
					}
				}
				cmd = item->_Cmd->GetReadonlyBuffer();
			}
			else
			{
				record();
				item->_CmdStaticInit[currentFrame] = true;
			}
		}
		else
		{
			record();
		}
		item->Submit(cmd);
		if (item->_LastRunCount == 0)
			item->_LastRunCount = currentTime;
		item->_DTime = double(currentTime - item->_LastRunCount) / 1.0e+9;
		item->_LastRunCount = currentTime;
	}
}

void egx::scene::ISceneComponent::StateHasChanged()
{
	_SceneData->SetSceneData("ISceneComponent::StateHasChanged", true);
}

void egx::scene::IScene::ReInitialize()
{
	std::fill(_CmdStaticInit.begin(), _CmdStaticInit.end(), false);
	for (auto& c : _SceneComponents)
	{
		try {
			bool firstLoad = false;
			if (!std::ranges::any_of(_ComponentsFirstInitialized, [&](size_t ptr) { return ptr == size_t(c.get()); })) {
				firstLoad = true;
			}
			c->OnInitialize(firstLoad);
			_ReadyComponents[size_t(c.get())] = true;
			if (firstLoad) {
				LOG(INFO, "Called OnInitialize(firstLoad: true) on Component {0}", typeid(*c).name());
				_ComponentsFirstInitialized.push_back(size_t(c.get()));
			}
		}
		catch (std::exception e) {
			LOG(WARNING, "Could not reload scene component {0} because {1}", typeid(*c).name(), e.what());
			_ReadyComponents[size_t(c.get())] = false;
		}
	}
}
void egx::scene::IScene::InternalResizeFramebuffer()
{
	if (_Framebuffer->Width == _Width && _Framebuffer->Height == _Height) return;
	_Framebuffer = OnCreateFramebuffer(_Width, _Height);
	// reinit components with new framebuffer
	ReInitialize();
	_ResizeFramebuffer = false;
}
