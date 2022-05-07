#pragma once
#include <functional>
#include <vector>

namespace job {

	typedef void(_cdecl* WorkFunction)(void* parameters);
	typedef void* BARRIER;

	struct Work {
		Work() {}
		Work(void* barrier, WorkFunction function, void* parameters) : mBarrier(barrier), mFunction(function), mParameters(parameters) {}
		void* mBarrier = nullptr;
		WorkFunction mFunction = nullptr;
		void* mParameters = nullptr;
	};

	extern int NumberOfThreads;

	void OnStartup();
	void OnDestroy();

	BARRIER CreateBarrier();
	void DestroyBarrier(BARRIER barrier);

	void NewFrame();
	void Execute(void* barrier, WorkFunction function, void* parameters);
	void CompleteSyncronization();

}
