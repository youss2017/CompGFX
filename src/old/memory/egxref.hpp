#pragma once
#include <cstdint>
#include <cassert>
#include <memory>
#include <atomic>

namespace egx {

	// template<typename T>
	// using ref = std::shared_ptr<T>;

	class RefCountedObject {
	protected:

		int IncrementReference() {
			return m_RefCount.fetch_add(1, std::memory_order_consume) + 1;
		}

		int DecrementReference() {
			return m_RefCount.fetch_sub(1, std::memory_order_consume) - 1;
		}

		int GetReferenceCount() {
			return m_RefCount.load();
		}

	private:
		std::atomic<int> m_RefCount;
	};

#if 1
	template<typename T>
	class ref {
	public:
		inline ref<T>(T* base) noexcept {
			this->base = base;
			refCount = new int32_t;
			*refCount = 1;
		}

		static inline ref<T> SelfReference(T* Self)
		{
			ref<T> _s;
			_s.base = Self;
			_s.refCount = nullptr;
			return _s;
		}

		inline ref<T>() noexcept {
			this->base = nullptr;
			refCount = nullptr;
		}

		inline ref<T>(const ref& cp) noexcept {
			this->base = cp.base;
			this->refCount = cp.refCount;
			if (cp.refCount) {
				*this->refCount += 1;
			}
		}

		inline ref<T>(ref&& move) noexcept :
			base(std::exchange(move.base, nullptr)),
			refCount(std::exchange(move.refCount, nullptr))
		{}

		inline ref<T>& operator=(const ref& cp) noexcept {
			if (cp.base == nullptr || &cp == this) {
				return *this;
			}
			this->base = cp.base;
			this->refCount = cp.refCount;
			if (cp.refCount)
				*this->refCount += 1;
			return *this;
		}

		inline ref<T>& operator=(ref&& move) noexcept {
			if (this == &move) return *this;

			if (refCount) {
				*refCount -= 1;
				if (*refCount == 0) {
					delete (T*)base;
					delete refCount;
				}
			}

			base = std::exchange(move.base, nullptr);
			refCount = std::exchange(move.refCount, nullptr);

			return *this;
		}

		constexpr T* operator->() const noexcept {
			return base;
		}

		constexpr T& operator*() const noexcept {
			return *base;
		}

		constexpr  T* operator()() const noexcept {
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

		inline bool IsValidRef() const noexcept { return (base != nullptr) && refCount && (*refCount > 0); }

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
		template<typename Src, typename Dst>
		friend ref<Dst> Cast(const ref<Src>& obj);
		int32_t* refCount;
	};
#endif

	template<typename Src, typename Dst>
	ref<Dst> Cast(const ref<Src>& obj) {
		ref<Dst> cast;
		cast.base = (Dst*)obj.base;
		cast.refCount = obj.refCount;
		*cast.refCount += 1;
		return cast;
	}

}
