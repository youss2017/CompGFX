#include "MT.hpp"
#include "Logger.hpp"
#include <Windows.h>

#define MTMAX(a, b) ((a > b) ? a : b)
#define MTMIN(a, b) ((a > b) ? b : a)
#define MAX_THREADS 64

FORCEINLINE MTOperation NTAPI AcquireMTOperation(MTContext* mt) {
	MTOperation op;
	op.pOperation = NULL;
	HANDLE hFence = (HANDLE)mt->hOperationSemaphore;
#if MT_MUTEX == 0
	EnterCriticalSection((LPCRITICAL_SECTION)&mt->w32CriticalSection);
#else
	HANDLE hOperationMutex = (HANDLE)mt->hOperationMutex;
	WaitForSingleObject(hOperationMutex, INFINITE);
#endif
	if (mt->vOperations.size() > 0) {
		op = mt->vOperations[0];
		mt->vOperations.pop_front();
	}
#if MT_MUTEX  == 0
	LeaveCriticalSection((LPCRITICAL_SECTION)&mt->w32CriticalSection);
#else
	ReleaseMutex(hOperationMutex);
#endif
	return op;
}

static DWORD NTAPI MTProc(LPVOID parameters) {
	MTParam* param = (MTParam*)parameters;
	// Wait until hThread is set.
	while (param->hThread == 0) {}
	MTContext* mt = param->pContext;
	int nThreadID = param->nThreadID;
	int nThreadCount = param->nThreadCount;
	HANDLE hSemaphore = (HANDLE)mt->hOperationSemaphore;
	while (true) {
		if (!mt->bRunning) {
			InterlockedExchange(&mt->nWorkInProgressCounter, 0);
			break;
		}
		MTOperation op = AcquireMTOperation(mt);
		if (op.pOperation) {
			op.pOperation(op.pOptionalArugments);
			InterlockedDecrement(&mt->nWorkInProgressCounter);
		}
		if (param->vOperations.size() > 0) {
			MTOperation op = param->vOperations[0];
			param->vOperations.pop_front();

			op.pOperationParrallel(nThreadID, nThreadCount, op.pOptionalArugments);
		}
		WaitForSingleObject(hSemaphore, INFINITE);
	}
	// Return 0xCC so we can determine if the thread exited safely.
	return 0xcc;
}

MTContext* MTCreate(int nCustomThreadCount) {
	int nThreadCount = nCustomThreadCount;

	if (nCustomThreadCount <= 0) {
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		nThreadCount = si.dwNumberOfProcessors;
		int mul = 1;
		if (nCustomThreadCount < 0) {
			mul = MTMIN(abs(nCustomThreadCount), 4);
		}
		nThreadCount *= mul;
	}
	nThreadCount = MTMIN(nThreadCount, MAX_THREADS);


	MTContext* mt = new MTContext();
	mt->nThreadCount = nThreadCount;
#if MT_MUTEX == 0
	InitializeCriticalSection((LPCRITICAL_SECTION)&mt->w32CriticalSection);
#else
	mt->hOperationMutex = (unsigned long long)CreateMutexA(nullptr, FALSE, "MT Operation Mutex");
#endif
	mt->hOperationSemaphore = (unsigned long long)CreateSemaphoreA(nullptr, 0, LONG_MAX, "MTContext_Semaphore");
	mt->bRunning = true;
	mt->nWorkInProgressCounter = 0;
	mt->nDelayStart = 0;
	mt->nDelayStartReserved = 0;

	mt->nAPIAccessMutex = (uint64_t)CreateMutexA(nullptr, FALSE, nullptr);

	mt->pThreads = new MTParam[nThreadCount];
	MTParam* pThreads = mt->pThreads;

	for (int i = 0; i < nThreadCount; i++) {
		MTParam* param = &pThreads[i];
		param->nThreadID = i;
		param->nThreadCount = nThreadCount;
		param->pContext = mt;
		HANDLE hThread = CreateThread(nullptr, 0, MTProc, param, CREATE_SUSPENDED, nullptr);
		if (hThread == INVALID_HANDLE_VALUE)
			continue;
		param->hThread = (unsigned long long)hThread;
#ifdef _DEBUG
		DWORD affinityMask = (1 << i);
		SetThreadAffinityMask(hThread, affinityMask);
#endif
		ResumeThread(hThread);
	}

	return mt;
}

void MTExecute(MTContext* mt, void* pMemoryBarrier, MTOperationFunc operation, void* pOptionalArguments) {
	WaitForSingleObject((HANDLE)mt->nAPIAccessMutex, INFINITE);
	MTOperation op;
	op.pOperation = operation;
	op.pOptionalArugments = pOptionalArguments;
#if MT_MUTEX  == 0
	EnterCriticalSection((LPCRITICAL_SECTION)&mt->w32CriticalSection);
#else
	HANDLE hOperationMutex = (HANDLE)mt->hOperationMutex;
	WaitForSingleObject(hOperationMutex, INFINITE);
#endif
	mt->vOperations.push_back(op);
#if MT_MUTEX  == 0
	LeaveCriticalSection((LPCRITICAL_SECTION)&mt->w32CriticalSection);
#else
	ReleaseMutex(hOperationMutex);
#endif
	if (mt->nDelayStart == 0) {
		InterlockedIncrement(&mt->nWorkInProgressCounter);
		ReleaseSemaphore((HANDLE)mt->hOperationSemaphore, mt->nThreadCount, nullptr);
	}
	else {
		mt->nDelayStartReserved += 1;
	}
	ReleaseMutex((HANDLE)mt->nAPIAccessMutex);
}

void MTExecuteParrallel(MTContext* mt, void* pMemoryBarrier, MTOperationParrallelFunc operation, void* pOptionalArguments) {
	WaitForSingleObject((HANDLE)mt->nAPIAccessMutex, INFINITE);
	MTOperation op;
	op.pOperationParrallel = operation;
	op.pOptionalArugments = pOptionalArguments;
	for (int i = 0; i < mt->nThreadCount; i++) {
		mt->pThreads[i].vOperations.push_back(op);
	}
	if (mt->nDelayStart == 0) {
		InterlockedAdd((volatile LONG*)&mt->nWorkInProgressCounter, mt->nThreadCount);
		ReleaseSemaphore((HANDLE)mt->hOperationSemaphore, mt->nThreadCount, nullptr);
	}
	else {
		mt->nDelayStartReserved += mt->nThreadCount;
	}
	ReleaseMutex((HANDLE)mt->nAPIAccessMutex);
}

void MTSetDelayStart(MTContext* mt, int state) {
	WaitForSingleObject((HANDLE)mt->nAPIAccessMutex, INFINITE);
	if (state != 0)
		MTSynchronize(mt);
	mt->nDelayStart = state;
	ReleaseMutex((HANDLE)mt->nAPIAccessMutex);
}

void MTDelayStart(MTContext* mt) {
	WaitForSingleObject((HANDLE)mt->nAPIAccessMutex, INFINITE);
	if (mt->nDelayStart == 0 || mt->nDelayStartReserved == 0)
		return;
	InterlockedAdd((volatile LONG*)&mt->nWorkInProgressCounter, mt->nDelayStartReserved);
	ReleaseSemaphore((HANDLE)mt->hOperationSemaphore, mt->nDelayStartReserved, nullptr);
	mt->nDelayStartReserved = 0;
	ReleaseMutex((HANDLE)mt->nAPIAccessMutex);
}

void MTSynchronize(MTContext* mt) {
	WaitForSingleObject((HANDLE)mt->nAPIAccessMutex, INFINITE);

sync_loop:
	SwitchToThread();
	if (InterlockedCompareExchangeNoFence(&mt->nWorkInProgressCounter, 0, 0) == 0) {
		goto quit_sync;
	}
	else {
		goto sync_loop;
	}
quit_sync:
	ReleaseMutex((HANDLE)mt->nAPIAccessMutex);
}

void MTDestroy(MTContext* context, int timeout) {
	WaitForSingleObject((HANDLE)context->nAPIAccessMutex, INFINITE);
	HANDLE hOperationSemaphore = (HANDLE)context->hOperationSemaphore;
	context->bRunning = false;
	ReleaseSemaphore(hOperationSemaphore, context->nThreadCount, nullptr);
	CloseHandle(hOperationSemaphore);
#if MT_MUTEX  != 0
	HANDLE hOperationMutex = (HANDLE)context->hOperationMutex;
	CloseHandle(hOperationMutex);
#endif
	MTParam* pThreads = (MTParam*)context->pThreads;
	for (int i = 0; i < context->nThreadCount; i++) {
		HANDLE hThread = (HANDLE)pThreads[i].hThread;
		if (timeout == 0) {
			WaitForSingleObject(hThread, INFINITE);
		}
		else {
			if (WaitForSingleObject(hThread, timeout) == WAIT_TIMEOUT) {
				TerminateThread(hThread, 0xea);
			}
		}
		CloseHandle(hThread);
	}
#if MT_MUTEX == 0
	DeleteCriticalSection((LPCRITICAL_SECTION)&context->w32CriticalSection);
#endif
	ReleaseMutex((HANDLE)context->nAPIAccessMutex);
	delete[] pThreads;
	delete context;
}
