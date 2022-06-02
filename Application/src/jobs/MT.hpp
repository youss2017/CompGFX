#pragma once
#include <deque>
#include <atomic>
// This option is only for Windows
// 0 Uses Critical Section instead of Mutex
// 1 Use Mutex instead of Critical Section
#define MT_MUTEX 0

using MTOperationFunc = void (*)(void* pArgs);
using MTOperationParrallelFunc = void(*)(int nThreadID, int nThreadCount, void* pArgs);

struct MTOperation {
	union {
		MTOperationFunc pOperation;
		MTOperationParrallelFunc pOperationParrallel;
	};
	void* pOptionalArugments;
};

struct MTParam {
	int nThreadID;
	int nThreadCount;
	unsigned long long hThread;
	struct MTContext* pContext;
	std::deque<MTOperation> vOperations;
};

struct MTContext {
	// threads allocated my context
	int nThreadCount;
	// memory allocation for each thread
	MTParam* pThreads;
	// Mutex so you can call MT...(mt, ...) functions from multiple threads.
	uint64_t nAPIAccessMutex;
#if MT_MUTEX == 1 || !defined(_WIN32)
	unsigned long long hOperationMutex;
#endif
	unsigned long long hOperationSemaphore;
	volatile uint32_t nWorkInProgressCounter;
	std::deque<MTOperation> vOperations;
	int nDelayStart;
	int nDelayStartReserved;
	volatile bool bRunning;
#if defined(_WIN32) && MT_MUTEX == 0
	struct {
		void* DebugInfo;
		long LockCount;
		long RecursionCount;
		void* OwningThread;
		void* LockSemaphore;
		unsigned long long SpinCount;
	} w32CriticalSection;
#endif
};

// custom == 0 means use system core count
// use negative number for multiples, eg -2 means [System Core Count] * 2 --- maximum of 4 or MAX_THREADS in MT.cpp
MTContext* MTCreate(int nCustomThreadCount);
// Instead of executing beginning after MTExecute..., we wait until MTDelayStart()
// This could be useful for measuring performance.
// To disable delay start set state==0
void MTSetDelayStart(MTContext* mt, int state);
// Begins execution if DelayState!=0 otherwise does nothing
void MTDelayStart(MTContext* mt);
void MTExecute(MTContext* mt, void* pMemoryBarrier, MTOperationFunc operation, void* pOptionalArguments);
void MTExecuteParrallel(MTContext* mt, void* pMemoryBarrier, MTOperationParrallelFunc operation, void* pOptionalArguments);
void MTSynchronize(MTContext* mt);
// timeout is measured in milliseconds and 0 means INFINITE
// timeout is time for thread to cleanup and exit. NOT TO FINISH WORK.
void MTDestroy(MTContext* context, int timeout);
