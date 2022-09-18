#pragma once
#include "egxcommon.hpp"
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <deque>
#include <condition_variable>

namespace egx {

    namespace _internal {
        struct AsyncWork {
            std::function<void(void*)> Work{};
            void* Args{};
        };

        struct WorkerSyncObject {
            std::mutex lock;
            std::condition_variable cond;
        };

        struct AsyncWorker {

        public:
            EGX_API AsyncWorker(
                WorkerSyncObject* HubSync,
                WorkerSyncObject* HubDone,
                std::deque<AsyncWork>* WorkQueue) :
                HubSync(HubSync),
                HubDone(HubDone),
                Active(true),
                WorkQueue(WorkQueue) {
                Self = std::make_unique<std::thread>(WorkLoop, this);
                Self->detach();
            }

            EGX_API ~AsyncWorker() {
                HubSync->cond.notify_all();
            }

        protected:
            static void EGX_API WorkLoop(AsyncWorker* worker) {
                while (worker->Active) {
                    AsyncWork work;
                    {
                        std::unique_lock<std::mutex> lock(worker->HubSync->lock);
                        worker->HubSync->cond.wait(lock, [&] {
                            if (worker->WorkQueue->size() == 0) {
                                worker->HubDone->cond.notify_all();
                            }
                            return (worker->WorkQueue->size() > 0) || !worker->Active;
                            });
                        if (!worker->Active || worker->WorkQueue->size() == 0) continue;
                        work = worker->WorkQueue->front();
                        worker->WorkQueue->pop_front();
                    }
                    work.Work(work.Args);
                }
            }

        public:
            bool Active;
            std::deque<AsyncWork>* WorkQueue;
            WorkerSyncObject* HubSync;
            WorkerSyncObject* HubDone;
        protected:
            std::unique_ptr<std::thread> Self;
        };
    }

    class AsyncWorkerHub {

    public:
        EGX_API AsyncWorkerHub() : ThreadCount(std::thread::hardware_concurrency()) {
            for (uint32_t i = 0; i < ThreadCount; i++) {
                this->Workers.push_back(std::make_unique<_internal::AsyncWorker>(&HubSync, &HubDone, &WorkQueue));
            }
        }

        EGX_API ~AsyncWorkerHub() {
            for (auto& worker : Workers) {
                worker->Active = false;
            }
        }

        void EGX_API RunAsync(std::function<void(void*)>&& work, void* args = nullptr, bool OwnThread = false) {
            if (!OwnThread) {
                std::lock_guard<std::mutex> lock(HubSync.lock);
                WorkQueue.push_back({ work, args });
            }
            else {
                std::thread thread(work, args);
                thread.detach();
            }
        }

    public:
        const uint32_t ThreadCount;
    protected:
        std::vector<std::unique_ptr<_internal::AsyncWorker>> Workers;
        std::deque<_internal::AsyncWork> WorkQueue;
        _internal::WorkerSyncObject HubSync;
        _internal::WorkerSyncObject HubDone;
    };

}