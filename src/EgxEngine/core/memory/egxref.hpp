#pragma once
#include <cstdint>
#include <cassert>

namespace egx {

	template<typename T>
	class ref {
	public:
		inline ref<T>(T* base) {
			this->base = base;
			refCount = new int32_t;
			*refCount = 1;
		}

		inline ref<T>(const T* base, bool SelfReference) {
			this->base = (T*)base;
			if (!SelfReference) {
				refCount = new int32_t;
				*refCount = 1;
			}
			else
				refCount = nullptr;
		}

		inline ref<T>() {
			this->base = nullptr;
			refCount = new int32_t;
			*refCount = 1;
		}

		inline ref<T>(const ref& cp) {
			this->base = cp.base;
			this->refCount = cp.refCount;
			*this->refCount += 1;		}

		inline ref<T>(ref&& move) {
			this->base = move.base;
			this->refCount = move.refCount;
			move.base = nullptr;
			move.refCount = nullptr;
		}

		inline ref<T>& operator=(ref move) {
			this->base = move.base;
			this->refCount = move.refCount;
			*this->refCount += 1;
			return *this;
		}

		inline T* operator->() const noexcept {
			return base;
		}

		inline T& operator*() const noexcept {
			return *base;
		}

		inline T* operator()() const noexcept {
			return base;
		}

		inline ~ref<T>() {
			if (refCount == nullptr) return;
			*refCount -= 1;
			// ref<T> is not thread safe
			// because refCount could drop below zero and deconstructor while not be called.
			// if we allow refCount < 0 to call deconstructor then multiple threads will call
			// the deconstructor
			assert(*refCount >= 0 && "Reference count is below 0. Are you using threading. ref<T> is not thread safe.");
			if (*refCount == 0) {
				delete (T*)base;
				delete refCount;
			}
		}

		inline bool IsValidRef() const noexcept { return (base != nullptr) && (*refCount > 0); }

		inline int32_t RefCount() const noexcept { return *refCount; }
		inline int32_t AddRef() noexcept { *refCount += 1; return *refCount; }

		inline bool DebugDeconstruction() {
			bool success = *refCount == 1;
			delete (T*)base;
			delete refCount;
			base = nullptr;
			refCount = nullptr;
			return success;
		}

	public:
		T* base;
	protected:
		int32_t* refCount;
	};

}
