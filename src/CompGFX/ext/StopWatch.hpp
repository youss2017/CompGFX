#pragma once
#include <chrono>

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

	public:
		std::chrono::steady_clock::time_point StartMeasurement;
		std::chrono::steady_clock::time_point StopMeasurement;
	};

}
