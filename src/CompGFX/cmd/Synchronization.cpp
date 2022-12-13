#include "synchronization.hpp"
#include "cmd.hpp"
#include <Utility/CppUtility.hpp>

using namespace egx;

ref<Semaphore>& ISynchronization::GetOrCreateCompletionSemaphore()
{
	if (_SignalSemaphore.IsValidRef()) return _SignalSemaphore;
	_SignalSemaphore = Semaphore::FactoryCreate(_CoreInterface, cpp::Format("ISynchronizationSignalSemaphore({0})", _ClassName));
	return _SignalSemaphore;
}

void ISynchronization::Submit(VkCommandBuffer cmd)
{
	std::vector<VkSemaphore> signalSemaphore;
	if (_SignalSemaphore.IsValidRef())
		signalSemaphore.push_back(_SignalSemaphore->GetSemaphore());
	CommandBuffer::Submit(_CoreInterface, { cmd },
		CommandBuffer::GetNativeSemaphoreList(_WaitSemaphores), _WaitStageFlags, signalSemaphore,
		_Completion.IsValidRef() ? _Completion->GetFence() : nullptr);

}