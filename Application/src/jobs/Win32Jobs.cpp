#ifdef _WIN32
#include "Jobs.hpp"
#include "Logger.hpp"
#include "atomic.h"
#include <assert.h>
#include <Windows.h>
#include <mutex>

namespace job {
	
	struct Win32Thread {
		HANDLE mThread;
		DWORD mThreadID;
	};

	int NumberOfThreads;
	static Win32Thread* sThreads;
	// Semaphore to prevent 100% utilization of the CPU
	static HANDLE sSemaphore;

	static volatile std::atomic<LONG> sWorkID;
	static volatile std::atomic<LONG> sWorkCount;
	static Work WorkItems[100];
	static volatile bool sRunning = true;
	std::mutex m;

	static DWORD __stdcall JobWin32Callback(LPVOID paramters) {
		while (sRunning) {
			m.lock();
			bool workVisible = sWorkID.load() < sWorkCount.load();
			int workID = sWorkID.load();
			if (workVisible)
				sWorkID.fetch_add(1);
			m.unlock();
			if (workVisible) {
				Work work = WorkItems[workID];
				if (work.mBarrier) {
					WaitForSingleObject(work.mBarrier, INFINITE);
				}
				work.mFunction(work.mParameters);
			}
			else {
				WaitForSingleObject(sSemaphore, INFINITE);
			}
		}
		return 0;
	}

	static inline LONG SemaphoreCount() {
		LONG count;
		ReleaseSemaphore(sSemaphore, 0, &count);
		return count;
	}

	void OnStartup() {
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		sWorkID = 0;
		sWorkCount = 0;
		NumberOfThreads = info.dwNumberOfProcessors - 1;
		sSemaphore = CreateSemaphoreA(nullptr, 0, INFINITE, "Win32JobSemaphore");
		sThreads = new Win32Thread[NumberOfThreads];
		for (int i = 0; i < NumberOfThreads; i++) {
			sThreads[i].mThread = CreateThread(nullptr, 0, JobWin32Callback, nullptr, 0, &sThreads[i].mThreadID);
			if (sThreads[i].mThread == nullptr) {
				logfatal("Could not create [Win32] Job System.", 0x8);
			}
		}
	}

	void OnDestroy() {
		sRunning = false;
		ReleaseSemaphore(sSemaphore, NumberOfThreads, nullptr);
		for (int i = 0; i < NumberOfThreads; i++) {
			if (WaitForSingleObject(sThreads[i].mThread, 100) == WAIT_TIMEOUT) {
				TerminateThread(sThreads[i].mThread, 0x55);
			}
			CloseHandle(sThreads[i].mThread);
		}
		CloseHandle(sSemaphore);
		delete[] sThreads;
	}

	void NewFrame() {
		CompleteSyncronization();
		sWorkID.store(0);
		sWorkCount.store(0);
	}

	void Execute(void* barrier, WorkFunction function, void* parameters) {
		m.lock();
		WorkItems[sWorkID.load()] = {barrier, function, parameters};
		sWorkCount.fetch_add(1);
		ReleaseSemaphore(sSemaphore, 1, nullptr);
		m.unlock();
	}

	void CompleteSyncronization() {
		if (SemaphoreCount() == 0)
			return;
		_ReadWriteBarrier();
		while (sWorkID.load() < sWorkCount.load()) {}
		sWorkID = 0;
		sWorkCount = 0;
	}

	BARRIER CreateBarrier() {
		return CreateSemaphoreA(nullptr, 0, 1, nullptr);
	}

	void DestroyBarrier(BARRIER barrier) {
		CloseHandle(barrier);
	}
}
#endif