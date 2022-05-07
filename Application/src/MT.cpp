#include "MT.hpp"
#include <Windows.h>

FORCEINLINE MTOperation NTAPI AcquireMTOperation(MTContext* mt) {
	HANDLE hOperationMutex = (HANDLE)mt->hOperationMutex;
	HANDLE hFence = (HANDLE)mt->hOperationSemaphore;
	WaitForSingleObject(hOperationMutex, INFINITE);
	MTOperation op;
	op.pOperation = NULL;
	if (mt->vOperations.size() > 0) {
		op = mt->vOperations[0];
		mt->vOperations.pop_front();
	}
	ReleaseMutex(hOperationMutex);
	return op;
}

static DWORD NTAPI MTProc(LPVOID parameters) {
	MTParam* param = (MTParam*)parameters;
	MTContext* mt = param->pContext;
	int nThreadID = param->nThreadID;
	int nThreadCount = param->nThreadCount;
	HANDLE hSemaphore = (HANDLE)mt->hOperationSemaphore;
	while (mt->bRunning) {
		WaitForSingleObject(hSemaphore, INFINITE);
		MTOperation op = AcquireMTOperation(mt);
		if (op.pOperation) {
			op.pOperation(op.pOptionalArugments);
		}
		if (param->vOperations.size() > 0) {
			MTOperation op = param->vOperations[0];
			param->vOperations.pop_front();
			op.pOperationParrallel(nThreadID, nThreadCount, op.pOptionalArugments);
		}
	}
	return 0;
}

MTContext* CreateMTContext() {
	int nThreadCount = 4;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	nThreadCount = si.dwNumberOfProcessors;

	MTContext* mt = new MTContext();
	mt->nThreadCount = nThreadCount;
	mt->hOperationMutex = (unsigned long long)CreateMutexA(nullptr, FALSE, "MTContext_Mutex");
	mt->hOperationSemaphore = (unsigned long long)CreateSemaphoreA(nullptr, 0, LONG_MAX, "MTContext_Semaphore");
	mt->bRunning = true;
	
	mt->pThreads = new MTParam[nThreadCount];
	MTParam* pThreads = mt->pThreads;

	DWORD_PTR affinity = ULONG_MAX;
	SetThreadAffinityMask(GetCurrentProcess(), affinity);

	for (int i = 0; i < nThreadCount; i++) {
		MTParam* param = &pThreads[i];
		param->nThreadID = i;
		param->nThreadCount = nThreadCount;
		param->pContext = mt;
		HANDLE hThread = CreateThread(nullptr, 0, MTProc, param, CREATE_SUSPENDED, nullptr);
		if (hThread == INVALID_HANDLE_VALUE)
			continue;
		param->hThread = (unsigned long long)hThread;
		affinity = ULONG_MAX;
		SetThreadAffinityMask(hThread, affinity);
		ResumeThread(hThread);
	}

	return mt;
}

void MTExecute(MTContext* mt, void* pMemoryBarrier, MTOperationFunc operation, void* pOptionalArguments) {
	HANDLE hOperationMutex = (HANDLE)mt->hOperationMutex;
	MTOperation op;
	op.pOperation = operation;
	op.pOptionalArugments = pOptionalArguments;
	WaitForSingleObject(hOperationMutex, INFINITE);
	mt->vOperations.push_back(op);
	ReleaseMutex(hOperationMutex);
	ReleaseSemaphore((HANDLE)mt->hOperationSemaphore, 1, nullptr);
}

void MTExecuteParrallel(MTContext* mt, void* pMemoryBarrier, MTOperationParrallelFunc operation, void* pOptionalArguments) {
	MTOperation op;
	op.pOperationParrallel = operation;
	op.pOptionalArugments = pOptionalArguments;
	for (int i = 0; i < mt->nThreadCount; i++) {
		mt->pThreads[i].vOperations.push_back(op);
	}
	ReleaseSemaphore((HANDLE)mt->hOperationSemaphore, mt->nThreadCount, nullptr);
}

void MTSynchronize(MTContext* mt)
{
	HANDLE hOperationSemaphore = (HANDLE)mt->hOperationSemaphore;
	LONG semaphoreCount = LONG_MAX;
	while (semaphoreCount > 0) {
		ReleaseSemaphore(hOperationSemaphore, 0, &semaphoreCount);
		Sleep(1);
	}
}


void DestroyMTContext(MTContext* context) {
	HANDLE hOperationMutex = (HANDLE)context->hOperationMutex;
	HANDLE hOperationSemaphore = (HANDLE)context->hOperationSemaphore;
	context->bRunning = false;
	ReleaseSemaphore(hOperationSemaphore, context->nThreadCount, nullptr);
	CloseHandle(hOperationSemaphore);
	CloseHandle(hOperationMutex);
	MTParam* pThreads = (MTParam*)context->pThreads;
	for (int i = 0; i < context->nThreadCount; i++) {
		HANDLE hThread = (HANDLE)pThreads[i].hThread;
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}
	delete[] pThreads;
	delete context;
}
