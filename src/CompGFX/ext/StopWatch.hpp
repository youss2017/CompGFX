#pragma once
#include <chrono>
#include <thread>
#include <functional>

namespace egx {

	class StopWatch {
	public:
		StopWatch() {
			Reset();
		}

		inline void Reset() {
			StartMeasurement = StopMeasurement = std::chrono::high_resolution_clock::now();
		}

		inline size_t Stop() {
			StopMeasurement = std::chrono::high_resolution_clock::now();
			return (StopMeasurement - StartMeasurement).count();
		}

		inline double TimeAsSeconds() const {
			return (StopMeasurement - StartMeasurement).count() * 1e-9;
		}

		inline double TimeAsMilliseconds() const {
			return (StopMeasurement - StartMeasurement).count() * 1e-6;
		}

		inline double TimeAsMicroseconds() const {
			return (StopMeasurement - StartMeasurement).count() * 1e-3;
		}

		inline size_t TimeAsNanoseconds() const {
			return (StopMeasurement - StartMeasurement).count();
		}

		inline double RatePerSecond() const {
			return 1e9 / double(TimeAsNanoseconds());
		}

		inline bool TriggerSeconds(double seconds) {
			auto time_point = std::chrono::high_resolution_clock::now();
			if ((time_point - StartMeasurement).count() >= (seconds * 1e9)) {
				StartMeasurement = time_point;
				return true;
			}
			return false;
		}

		inline bool TriggerMilliseconds(double milliseconds) {
			auto time_point = std::chrono::high_resolution_clock::now();
			if ((time_point - StartMeasurement).count() >= (milliseconds * 1e6)) {
				StartMeasurement = time_point;
				return true;
			}
			return false;
		}

		inline bool TriggerMicroseconds(double microseconds) {
			auto time_point = std::chrono::high_resolution_clock::now();
			if ((time_point - StartMeasurement).count() >= (microseconds * 1e3)) {
				StartMeasurement = time_point;
				return true;
			}
			return false;
		}

		static void CreateEventCallback(double seconds, const std::function<bool()>& callback) {
			auto thread = std::thread([=] {
				bool recursive = false;
				do {
					std::this_thread::sleep_for(std::chrono::nanoseconds(uint64_t(seconds * 1e9)));
					recursive = callback();
				} while (recursive);
			});
			thread.detach();
		}

	public:
		std::chrono::steady_clock::time_point StartMeasurement;
		std::chrono::steady_clock::time_point StopMeasurement;
	};

}
