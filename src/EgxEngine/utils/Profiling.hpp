#pragma once
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <util/Logger.hpp>

// Quick Function to check performance

#define CheckActionTime(Action) \
{ \
    auto CheckActionTime_START = std::chrono::high_resolution_clock::now();\
    Action;\
    auto CheckActionTime_END = std::chrono::high_resolution_clock::now();\
    double CheckActionTime_DURATION = (CheckActionTime_END - CheckActionTime_START).count();\
    CheckActionTime_DURATION *= 1e-6;\
    std::cout << __FILE__ << ":" << __LINE__ << " --- " << #Action << " Took " << CheckActionTime_DURATION << " ms\n";\
}

#define CheckBlockTimeBegin()\
{\
	auto CheckBlockTime_START = std::chrono::high_resolution_clock::now();

#define CheckBlockTimeEnd(BlockName)\
    auto CheckBlockTime_END = std::chrono::high_resolution_clock::now();\
    double CheckBlockTime_DURATION = (CheckBlockTime_END - CheckBlockTime_START).count();\
    CheckBlockTime_DURATION *= 1e-6;\
    std::cout << __FILE__ << ":" << __LINE__ << " --- " << BlockName << " Took " << CheckBlockTime_DURATION << " ms\n";\
}


// Code from CHERNO Game Engine

#define PROFILE_ENABLED 0 // default == 0

#if PROFILE_ENABLED == 2
#define __PROFILE_RESULT_NAME G_PROFILE_RESULTS
#define PROFILE_RESULTS Utils :: __PROFILE_RESULT_NAME

#define __PROFILE_TOKENPASTE(x, y) x##y
#define __PROFILE_TOKENPASTE2(x, y) __PROFILE_TOKENPASTE(x, y)
#define PROFILE_SCOPE(name) Utils::Timer __PROFILE_TOKENPASTE2(__PROFILE_TOKENPASTE2(timer_, __LINE__)_, __COUNTER__) \
	(name, [&](Utils::ProfileResult result) \
		{\
			PROFILE_RESULTS.push_back(result);\
		}\
	)

#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)
#endif

namespace Utils
{
#if PROFILE_ENABLED == 2
	struct ProfileResult
	{
		char m_Name[80]; // reducing heap allocations
		float m_Duration; // in ms
	};

	template <typename Fn>
	class Timer
	{

	public:
		Timer(const char *name, Fn &&func)
			: m_Func(func), m_Stopped(false)
		{
			strcpy(m_Name, name);
			m_StartTimePoint = std::chrono::high_resolution_clock::now();
		}

		~Timer()
		{
			if (!m_Stopped)
				Stop();
		}

		void Stop()
		{
			auto EndTimePoint = std::chrono::high_resolution_clock::now();

			uint64_t start = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_StartTimePoint).time_since_epoch().count();
			uint64_t end = std::chrono::time_point_cast<std::chrono::nanoseconds>(EndTimePoint).time_since_epoch().count();

			m_Stopped = true;

			float duration = (float)(end - start) * 1e-6f;

			// std::cout << "[" << m_Name << "]: " << duration << " ms\n";
			//  Call Fn with ProfileResult
			ProfileResult rt;
			strcpy(rt.m_Name, m_Name);
			rt.m_Duration = duration;
			m_Func(rt);
		}

	private:
		char m_Name[80]; // better than std::string because its stack allocation instead
		Fn m_Func;
		bool m_Stopped;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimePoint;
	};

	extern std::vector<ProfileResult> __PROFILE_RESULT_NAME;

#else
	// from https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Debug/Instrumentor.h
	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

	struct ProfileResult
	{
		std::string Name;

		FloatingPointMicroseconds Start;
		std::chrono::microseconds ElapsedTime;
		std::thread::id ThreadID;
	};

	struct InstrumentationSession
	{
		std::string Name;
	};

	class Instrumentor
	{
	public:
		Instrumentor(const Instrumentor &) = delete;
		Instrumentor(Instrumentor &&) = delete;

		void BeginSession(const std::string &name, const std::string &filepath = "results.json")
		{
			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession)
			{
				// If there is already a current session, then close it before beginning new one.
				// Subsequent profiling output meant for the original session will end up in the
				// newly opened session instead.  That's better than having badly formatted
				// profiling output.
				std::string err_msg = std::string("Instrumentor::BeginSession('") + std::string(name) + std::string("') when session '") + std::string(m_CurrentSession->Name) + "' already open.";
				ut::log_error(err_msg.c_str(), __FILE__, __LINE__);
				InternalEndSession();
			}
			m_OutputStream.open(filepath);

			if (m_OutputStream.is_open())
			{
				m_CurrentSession = new InstrumentationSession({name});
				WriteHeader();
			}
			else
			{
				std::string err_msg = std::string("Instrumentor could not open results file '") + filepath;
				ut::log_error(err_msg.c_str(), __FILE__, __LINE__);
			}
		}

		void EndSession()
		{
			std::lock_guard lock(m_Mutex);
			InternalEndSession();
		}

		void WriteProfile(const ProfileResult &result)
		{
			std::stringstream json;

			json << std::setprecision(3) << std::fixed;
			json << ",{";
			json << "\"cat\":\"function\",";
			json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
			json << "\"name\":\"" << result.Name << "\",";
			json << "\"ph\":\"X\",";
			json << "\"pid\":0,";
			json << "\"tid\":" << result.ThreadID << ",";
			json << "\"ts\":" << result.Start.count();
			json << "}";

			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession)
			{
				m_OutputStream << json.str();
				m_OutputStream.flush();
			}
		}

		static Instrumentor &Get()
		{
			static Instrumentor instance;
			return instance;
		}

	private:
		Instrumentor()
			: m_CurrentSession(nullptr)
		{
		}

		~Instrumentor()
		{
			EndSession();
		}

		void WriteHeader()
		{
			m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
			m_OutputStream.flush();
		}

		void WriteFooter()
		{
			m_OutputStream << "]}";
			m_OutputStream.flush();
		}

		// Note: you must already own lock on m_Mutex before
		// calling InternalEndSession()
		void InternalEndSession()
		{
			if (m_CurrentSession)
			{
				WriteFooter();
				m_OutputStream.close();
				delete m_CurrentSession;
				m_CurrentSession = nullptr;
			}
		}

	private:
		std::mutex m_Mutex;
		InstrumentationSession *m_CurrentSession;
		std::ofstream m_OutputStream;
	};

	class InstrumentationTimer
	{
	public:
		InstrumentationTimer(const char *name)
			: m_Name(name), m_Stopped(false)
		{
			m_StartTimepoint = std::chrono::steady_clock::now();
		}

		~InstrumentationTimer()
		{
			if (!m_Stopped)
				Stop();
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::steady_clock::now();
			auto highResStart = FloatingPointMicroseconds{m_StartTimepoint.time_since_epoch()};
			auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

			Instrumentor::Get().WriteProfile({m_Name, highResStart, elapsedTime, std::this_thread::get_id()});

			m_Stopped = true;
		}

	private:
		const char *m_Name;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
		bool m_Stopped;
	};

	namespace InstrumentorUtils
	{

		template <size_t N>
		struct ChangeResult
		{
			char Data[N];
		};

		template <size_t N, size_t K>
		constexpr auto CleanupOutputString(const char (&expr)[N], const char (&remove)[K])
		{
			ChangeResult<N> result = {};

			size_t srcIndex = 0;
			size_t dstIndex = 0;
			while (srcIndex < N)
			{
				size_t matchIndex = 0;
				while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
					matchIndex++;
				if (matchIndex == K - 1)
					srcIndex += matchIndex;
				result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
				srcIndex++;
			}
			return result;
		}
	}
#endif
}

#if PROFILE_ENABLED == 1
	// Resolve which function signature macro will be used. Note that this only
	// is resolved when the (pre)compiler starts, so the syntax highlighting
	// could mark the wrong one in your editor!
	#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
		#define HZ_FUNC_SIG __PRETTY_FUNCTION__
	#elif defined(__DMC__) && (__DMC__ >= 0x810)
		#define HZ_FUNC_SIG __PRETTY_FUNCTION__
	#elif (defined(__FUNCSIG__) || (_MSC_VER))
		#define HZ_FUNC_SIG __FUNCSIG__
	#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
		#define HZ_FUNC_SIG __FUNCTION__
	#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
		#define HZ_FUNC_SIG __FUNC__
	#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
		#define HZ_FUNC_SIG __func__
	#elif defined(__cplusplus) && (__cplusplus >= 201103)
		#define HZ_FUNC_SIG __func__
	#else
		#define HZ_FUNC_SIG "HZ_FUNC_SIG unknown!"
	#endif

	#define PROFILE_BEGIN_SESSION(name, filepath) ::Utils::Instrumentor::Get().BeginSession(name, filepath)
	#define PROFILE_END_SESSION() ::Utils::Instrumentor::Get().EndSession()
	#define PROFILE_SCOPE_LINE2(name, line) constexpr auto fixedName##line = ::Utils::InstrumentorUtils::CleanupOutputString(name, "__cdecl ");\
											   ::Utils::InstrumentationTimer timer##line(fixedName##line.Data)
	#define PROFILE_SCOPE_LINE(name, line) PROFILE_SCOPE_LINE2(name, line)
	#define PROFILE_SCOPE(name) PROFILE_SCOPE_LINE(name, __LINE__)
	#define PROFILE_FUNCTION() PROFILE_SCOPE(HZ_FUNC_SIG)
#elif PROFILE_ENABLED == 0
	#define PROFILE_BEGIN_SESSION(name, filepath)
	#define PROFILE_END_SESSION()
	#define PROFILE_SCOPE(name)
	#define PROFILE_FUNCTION()
#elif PROFILE_ENABLED == 2
	#define PROFILE_BEGIN_SESSION(name, filepath)
	#define PROFILE_END_SESSION()
#else
#error "Invalid PROFILE_ENABLE"
#endif
