#pragma once
#include <deque>

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
	int nThreadCount;
	MTParam* pThreads;
	unsigned long long hOperationMutex;
	unsigned long long hOperationSemaphore;
	std::deque<MTOperation> vOperations;
	bool bRunning;
};

MTContext* CreateMTContext();
void DestroyMTContext(MTContext* context);
void MTExecute(MTContext* mt, void* pMemoryBarrier, MTOperationFunc operation, void* pOptionalArguments);
void MTExecuteParrallel(MTContext* mt, void* pMemoryBarrier, MTOperationParrallelFunc operation, void* pOptionalArguments);
void MTSynchronize(MTContext* mt);
