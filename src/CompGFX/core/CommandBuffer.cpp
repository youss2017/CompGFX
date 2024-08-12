#include "CommandBuffer.hpp"
using namespace std;

void egx::ICommandBuffer::Invalidate(const DeviceCtx& device)
{
	_data = make_shared<ICommandBuffer::DataWrapper>(device);
}

void egx::ICommandBuffer::Reset()
{
	_data->_Reset();
}

vk::CommandBuffer egx::ICommandBuffer::GetCmd(const std::string& name) {
	return _data->_GetCmd(name);
}

egx::ICommandBuffer::DataWrapper::DataWrapper(const DeviceCtx& device) : _device(device)
{
	for (uint32_t i = 0; i < device->FramesInFlight; i++) {
		auto poolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(device->GraphicsQueueFamilyIndex);
		Pool p{};
		p._Pool = device->Device.createCommandPool(poolInfo);
		_Pools.push_back(p);
		if (!p._Pool) {
			// How should we recover here?
			throw runtime_error("Could not create command pool.");
		}
		
	}
}

vk::CommandBuffer egx::ICommandBuffer::Pool::GetOrCreateCmd(const DeviceCtx& device, const std::string& name)
{
	auto cmd = _Cmds.find(name);
	if (cmd != _Cmds.end()) {
		return cmd->second;
	}
	auto cmdInfo = vk::CommandBufferAllocateInfo()
		.setCommandPool(_Pool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(1);
	auto cmds = device->Device.allocateCommandBuffers(cmdInfo);
	if (cmds.empty()) {
		throw runtime_error("Could not allocate command buffer from pool.");
	}
	_Cmds[name] = cmds[0];
	return _Cmds[name];
}

egx::ICommandBuffer::DataWrapper::~DataWrapper()
{
	for (auto& pool : _Pools) {
		_device->Device.destroyCommandPool(pool._Pool);
	}
}

vk::CommandBuffer egx::ICommandBuffer::DataWrapper::_GetCmd(const std::string& name)
{
	return _Pools[_device->CurrentFrame].GetOrCreateCmd(_device, name);
}

void egx::ICommandBuffer::DataWrapper::_Reset()
{
	_device->Device.resetCommandPool(_Pools[_device->CurrentFrame]._Pool);
}

void egx::IFramedFence::Invalidate(const DeviceCtx& device)
{
	_data = make_shared<IFramedFence::DataWrapper>(device);
}

vk::Fence egx::IFramedFence::GetFence(bool createSignaled, const std::string& name)
{
	return _data->_GetFence(createSignaled, name);
}

void egx::IFramedFence::Wait(const std::string& name)
{
	_data->_Device->Device.waitForFences(_data->_GetFence(true, name), true, UINT64_MAX);
}

void egx::IFramedFence::Reset(const std::string& name)
{
	_data->_Device->Device.resetFences(_data->_GetFence(true, name));
}

egx::IFramedFence::DataWrapper::DataWrapper(const DeviceCtx& device) : _Device(device) {}

egx::IFramedFence::DataWrapper::DataWrapper::~DataWrapper() {
	for (auto& [name, fences] : _Fences) {
		for (vk::Fence fence : fences)
			_Device->Device.destroyFence(fence);
	}
}

vk::Fence egx::IFramedFence::DataWrapper::_GetFence(bool createSignaled, const std::string& name)
{
	auto found = _Fences.find(name);
	if (found != _Fences.end()) {
		return found->second[_Device->CurrentFrame];
	}
	std::vector<vk::Fence> fences(_Device->FramesInFlight);
	for (uint32_t i = 0; i < _Device->FramesInFlight; i++) {
		fences[i] = _Device->Device.createFence(vk::FenceCreateInfo()
													.setFlags(createSignaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags(0)));
	}
	_Fences[name] = fences;
	return fences[_Device->CurrentFrame];
}


void egx::IFramedSemaphore::Invalidate(const DeviceCtx& device)
{
	_data = make_shared<IFramedSemaphore::DataWrapper>(device);
}

vk::Semaphore egx::IFramedSemaphore::GetSemaphore(const std::string& name)
{
	return _data->_GetSemaphore(name);
}

egx::IFramedSemaphore::DataWrapper::DataWrapper(const DeviceCtx& device) : _Device(device) {}

egx::IFramedSemaphore::DataWrapper::DataWrapper::~DataWrapper() {
	for (auto& [name, semaphores] : _Semaphores) {
		for(vk::Semaphore semaphore : semaphores)
			_Device->Device.destroySemaphore(semaphore);
	}
}

vk::Semaphore egx::IFramedSemaphore::DataWrapper::_GetSemaphore(const std::string& name)
{
	auto found = _Semaphores.find(name);
	if (found != _Semaphores.end()) {
		return found->second[_Device->CurrentFrame];
	}
	std::vector<vk::Semaphore> semaphores(_Device->FramesInFlight);
	for (uint32_t i = 0; i < _Device->FramesInFlight; i++) {
		semaphores[i] = _Device->Device.createSemaphore({});
	}
	_Semaphores[name] = semaphores;
	return semaphores[_Device->CurrentFrame];
}
