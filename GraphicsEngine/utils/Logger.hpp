#pragma once

#if !defined(_DEBUG)
//#define NO_LOGGER
#endif

#ifdef GRAPHICS_API
#undef GRAPHICS_API
#endif

#ifdef BUILD_GRAPHICS_DLL
#if BUILD_GRAPHICS_DLL == 1
#define GRAPHICS_API _declspec(dllexport)
#else
#define GRAPHICS_API _declspec(dllimport)
#endif
#else
#define GRAPHICS_API
#endif

extern GRAPHICS_API void log_configure(bool EnableLogger, bool UseDateTime, bool EnableFileLogging = false, const char* customFileName = 0);
extern GRAPHICS_API void log_raw(const char* msg, bool ShowMessageBox);
extern GRAPHICS_API void log_info(const char* msg, bool ShowMessageBox);
extern GRAPHICS_API void log_warning(const char* msg, bool ShowMessageBox);
extern GRAPHICS_API void log_alert(const char* msg, bool ShowMessageBox);
extern GRAPHICS_API bool log_error(const char* msg, const char* file = "N/A", int line = 0, bool show_message_box = true);
extern GRAPHICS_API void log_fatal(int exit_code, const char* msg, const char* file, int line);

#define lograw(x) log_raw(x, false)
#define lograws(x) lograw(x.c_str())

#define loginfo(x) log_info(x, false)
#define loginfos(x) loginfo(x.c_str())

#define logwarning(x) log_warning(x, false)
#define logwarnings(x) logwarning(x.c_str())

#define logalert(x) log_alert(x, false)
#define logalerts(x) logalert(x.c_str());

#define logerror(msg) log_error(msg, __FILE__, __LINE__)
#define logerrors(msg) logerror(msg.c_str())
#define logerror_break(msg) if(!log_error(msg, __FILE__, __LINE__)) Utils::Break();
#define logerrors_break(msg) logerror_break(msg.c_str());

#define logfatal(msg, exit_code)  log_fatal(exit_code, msg, __FILE__, __LINE__);
#define logfatals(msg, exit_code) log_fatal(exit_code, msg.c_str(), __FILE__, __LINE__);
