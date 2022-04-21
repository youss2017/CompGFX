#include "Logger.hpp"
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <string>
#include <stdio.h>
#include <chrono>
#include <string.h>
#include "MBox.hpp"
#include "StringUtils.hpp"
#include "Profiling.hpp"

/*
Source: https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences#4842446
Code	Effect	Note
0	Reset / Normal	all attributes off
1	Bold or increased intensity
2	Faint (decreased intensity)	Not widely supported.
3	Italic	Not widely supported. Sometimes treated as inverse.
4	Underline
5	Slow Blink	less than 150 per minute
6	Rapid Blink	MS-DOS ANSI.SYS; 150+ per minute; not widely supported
7	[[reverse video]]	swap foreground and background colors
8	Conceal	Not widely supported.
9	Crossed-out	Characters legible, but marked for deletion. Not widely supported.
10	Primary(default) font
11–19	Alternate font	Select alternate font n-10
20	Fraktur	hardly ever supported
21	Bold off or Double Underline	Bold off not widely supported; double underline hardly ever supported.
22	Normal color or intensity	Neither bold nor faint
23	Not italic, not Fraktur
24	Underline off	Not singly or doubly underlined
25	Blink off
27	Inverse off
28	Reveal	conceal off
29	Not crossed out
30–37	Set foreground color	See color table below
38	Set foreground color	Next arguments are 5;<n> or 2;<r>;<g>;<b>, see below
39	Default foreground color	implementation defined (according to standard)
40–47	Set background color	See color table below
48	Set background color	Next arguments are 5;<n> or 2;<r>;<g>;<b>, see below
49	Default background color	implementation defined (according to standard)
51	Framed
52	Encircled
53	Overlined
54	Not framed or encircled
55	Not overlined
60	ideogram underline	hardly ever supported
61	ideogram double underline	hardly ever supported
62	ideogram overline	hardly ever supported
63	ideogram double overline	hardly ever supported
64	ideogram stress marking	hardly ever supported
65	ideogram attributes off	reset the effects of all of 60-64
90–97	Set bright foreground color	aixterm (not in standard)
100–107	Set bright background color	aixterm (not in standard)
*/

typedef enum POSIX_COLORS
{
    POSIX_COLOR_BLACK = 30,
    POSIX_COLOR_RED = 31,
    POSIX_COLOR_GREEN = 32,
    POSIX_COLOR_YELLOW = 33,
    POSIX_COLOR_BLUE = 34,
    POSIX_COLOR_MAGENTA = 35,
    POSIX_COLOR_CYAN = 36,
    POSIX_COLOR_WHITE = 37,
    POSIX_COLOR_BRIGHT_BLACK = 90,
    POSIX_COLOR_BRIGHT_RED = 91,
    POSIX_COLOR_BRIGHT_GREEN = 92,
    POSIX_COLOR_BRIGHT_YELLOW = 93,
    POSIX_COLOR_BRIGHT_BLUE = 94,
    POSIX_COLOR_BRIGHT_MAGENTA = 95,
    POSIX_COLOR_BRIGHT_CYAN = 96,
    POSIX_COLOR_BRIGHT_WHITE = 97,

    POSIX_BGCOLOR_BLACK = 40,
    POSIX_BGCOLOR_RED = 41,
    POSIX_BGCOLOR_GREEN = 42,
    POSIX_BGCOLOR_YELLOW = 43,
    POSIX_BGCOLOR_BLUE = 44,
    POSIX_BGCOLOR_MAGENTA = 45,
    POSIX_BGCOLOR_CYAN = 46,
    POSIX_BGCOLOR_WHITE = 47,
    POSIX_BGCOLOR_BRIGHT_BLACK = 100,
    POSIX_BGCOLOR_BRIGHT_RED = 101,
    POSIX_BGCOLOR_BRIGHT_GREEN = 102,
    POSIX_BGCOLOR_BRIGHT_YELLOW = 103,
    POSIX_BGCOLOR_BRIGHT_BLUE = 104,
    POSIX_BGCOLOR_BRIGHT_MAGENTA = 105,
    POSIX_BGCOLOR_BRIGHT_CYAN = 106,
    POSIX_BGCOLOR_BRIGHT_WHITE = 107,
} POSIX_COLORS;

/*Enum to store Foreground colors*/
typedef enum WIN32FG_COLORS
{
    WIN32FG_BLACK = 0,
    WIN32FG_BLUE = 1,
    WIN32FG_GREEN = 2,
    WIN32FG_CYAN = 3,
    WIN32FG_RED = 4,
    WIN32FG_MAGENTA = 5,
    WIN32FG_BROWN = 6,
    WIN32FG_LIGHTGRAY = 7,
    WIN32FG_GRAY = 8,
    WIN32FG_LIGHTBLUE = 9,
    WIN32FG_LIGHTGREEN = 10,
    WIN32FG_LIGHTCYAN = 11,
    WIN32FG_LIGHTRED = 12,
    WIN32FG_LIGHTMAGENTA = 13,
    WIN32FG_YELLOW = 14,
    WIN32FG_WHITE = 15
} WIN32FG_COLORS;

/*Enum to store Background colors*/
typedef enum WIN32BG_COLORS
{
    WIN32BG_NAVYBLUE = 16,
    WIN32BG_GREEN = 32,
    WIN32BG_TEAL = 48,
    WIN32BG_MAROON = 64,
    WIN32BG_PURPLE = 80,
    WIN32BG_OLIVE = 96,
    WIN32BG_SILVER = 112,
    WIN32BG_GRAY = 128,
    WIN32BG_BLUE = 144,
    WIN32BG_LIME = 160,
    WIN32BG_CYAN = 176,
    WIN32BG_RED = 192,
    WIN32BG_MAGENTA = 208,
    WIN32BG_YELLOW = 224,
    WIN32BG_WHITE = 240
} WIN32BG_COLORS;

#if defined(_WIN32)
static HANDLE ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#else
std::string GetColorPrefix(POSIX_COLORS ForegroundColor, POSIX_COLORS BackgroundColor);
#endif
using namespace std;

static auto ProgramStartTime = chrono::high_resolution_clock::now();

static bool LoggerEnabled = true;
static bool UseDateFormat = false;

static bool InternalFileLoggingEnabled = false;
static FILE *InternalFileHandle = NULL;

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
static const std::string GetCurrentDateTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}

void log_configure(bool EnableLogger, bool _UseDateFormat, bool EnableFileLogging, const char *customFileName)
{
    LoggerEnabled = EnableLogger;
    UseDateFormat = _UseDateFormat;
    if (EnableFileLogging)
    {
        InternalFileLoggingEnabled = true;
        string fileName = " ";
        if (customFileName)
        {
            fileName = customFileName;
        }
        else
        {
            std::string time_point = GetCurrentDateTime();
            time_point = Utils::StrReplaceAll(time_point, ".", " ");
            time_point = Utils::StrReplaceAll(time_point, ":", ".");
            fileName += to_string(time(NULL)) + "_" + time_point + ".txt";
        }
        InternalFileHandle = fopen(fileName.c_str(), "w");
        if (!InternalFileHandle)
        {
            InternalFileLoggingEnabled = false;
            log_warning("Could Create File for logger!", true);
        }
    }
    else
    {
        if (InternalFileHandle)
        {
            InternalFileLoggingEnabled = false;
            if (InternalFileHandle)
            {
                fflush(InternalFileHandle);
                fclose(InternalFileHandle);
            }
            InternalFileHandle = NULL;
        }
    }
}

#if !defined(_WIN32)
static string GetTimeStamp(POSIX_COLORS foregroundColor, POSIX_COLORS backgroundColor, int &prefixCount)
{
    string result;
    if (!UseDateFormat)
    {
        double now = (double)((chrono::high_resolution_clock::now() - ProgramStartTime).count()) / 1000000000.0; // seconds
        if (now > 120.0)
        {
            now = now / 60.0;
            if (now < 60.0)
            {
                result = "[" + to_string(now) + " min";
            }
            else
            {
                now = now / 60.0;
                result = "[" + to_string(now) + " hr";
            }
        }
        else
        {
            result = "[" + to_string(now) + " sec";
        }
        string prefix = GetColorPrefix(foregroundColor, backgroundColor);
        result = prefix + result;
        prefixCount = prefix.size();
    }
    else
    {
        result = "[" + GetCurrentDateTime();
    }
    return result;
}
#else
static string GetTimeStamp()
{
    string result;
    if (!UseDateFormat)
    {
        double now = (double)((chrono::high_resolution_clock::now() - ProgramStartTime).count()) / 1000000000.0; // seconds
        if (now > 120.0)
        {
            now = now / 60.0;
            if (now < 60.0)
            {
                result = "[" + to_string(now) + " min";
            }
            else
            {
                now = now / 60.0;
                result = "[" + to_string(now) + " hr";
            }
        }
        else
        {
            result = "[" + to_string(now) + " sec";
        }
    }
    else
    {
        result = "[" + GetCurrentDateTime();
    }
    return result;
}

#endif

#if !defined(NO_LOGGER)
void log_raw(const char *msg, bool ShowMessageBox)
{
    PROFILE_FUNCTION();
    if (!LoggerEnabled)
        return;
#if !defined(_WIN32)
    cout << "\033[1;31;33m" << msg << "\033[0m";
#else
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_LIGHTCYAN);
    cout << msg;
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_WHITE);
#endif
    if (ShowMessageBox)
    {
        Utils::Message("Custom", msg, Utils::INFO);
    }
    if (InternalFileLoggingEnabled)
    {
        fwrite(msg, 1, strlen(msg), InternalFileHandle);
        fflush(InternalFileHandle);
    }
}

void log_info(const char *msg, bool ShowMessageBox)
{
    PROFILE_FUNCTION();
    if (!LoggerEnabled)
        return;
    int prefixSize = 0;
    string message;
#if !defined(_WIN32)
    message = GetTimeStamp(POSIX_COLOR_BRIGHT_GREEN, POSIX_BGCOLOR_BLACK, prefixSize) + ", INFO]: ";
#else
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_GREEN);
    message = GetTimeStamp() + "]: ";
#endif
    message += msg;
#if !defined(_WIN32)
    message += "\n\033[0m";
    cout << message;
#else
    message += "\n";
    cout << message;
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_WHITE);
#endif
    message = message.substr(prefixSize);
    message = message.substr(0, message.find_last_of("\033"));
    if (ShowMessageBox)
        Utils::Message("Engine Information", message, Utils::INFO);
    if (InternalFileLoggingEnabled)
    {
        fwrite(message.c_str(), 1, message.size(), InternalFileHandle);
        fflush(InternalFileHandle);
    }
}

void log_warning(const char *msg, bool ShowMessageBox)
{
    PROFILE_FUNCTION();
    if (!LoggerEnabled)
        return;
    int prefixSize = 0;
#if !defined(_WIN32)
    string message = GetTimeStamp(POSIX_COLOR_YELLOW, POSIX_BGCOLOR_BLACK, prefixSize) + ", Warning]: ";
#else
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_YELLOW);
    string message = GetTimeStamp() + "]: ";
#endif
    message += msg;
#if !defined(_WIN32)
    message += "\n\033[0m";
    cout << message;
#else
    message += "\n";
    cout << message;
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_WHITE);
#endif
    message = message.substr(prefixSize);
    message = message.substr(0, message.find_last_of("\033"));
    if (ShowMessageBox)
        Utils::Message("Engine Warning", message, Utils::WARNING);
    if (InternalFileLoggingEnabled)
    {
        fwrite(message.c_str(), 1, message.size(), InternalFileHandle);
        fflush(InternalFileHandle);
    }
}

void log_alert(const char *msg, bool ShowMessageBox)
{
    PROFILE_FUNCTION();
    if (!LoggerEnabled)
        return;
    int prefixSize = 0;
#if !defined(_WIN32)
    string error_message = GetTimeStamp(POSIX_COLOR_BRIGHT_YELLOW, POSIX_BGCOLOR_BRIGHT_RED, prefixSize) + ", ALERT]: ";
    error_message += msg + GetColorPrefix(POSIX_COLOR_WHITE, POSIX_BGCOLOR_BLACK) + "\n";
#else
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_YELLOW | WIN32BG_RED);
    string error_message = GetTimeStamp() + "]: ";
    error_message += msg;
    error_message += "\n";
#endif
    cout << error_message;
#if !defined(_WIN32)
    error_message = error_message.substr(prefixSize);
    error_message = error_message.substr(0, error_message.find_last_of("\033"));
#else
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_WHITE);
#endif
    if (ShowMessageBox)
        Utils::Message("!!!ALERT!!!", error_message, Utils::WARNING);
    if (InternalFileLoggingEnabled)
    {
        fwrite(error_message.c_str(), 1, error_message.size(), InternalFileHandle);
        fflush(InternalFileHandle);
    }
}

#endif

bool log_error(const char *msg, const char *file, int line, bool show_message_box)
{
    PROFILE_FUNCTION();
    if (!LoggerEnabled)
        return false;
    int prefixSize = 0;
#if !defined(_WIN32)
    string error_message = GetTimeStamp(POSIX_COLOR_BRIGHT_RED, POSIX_BGCOLOR_BLACK, prefixSize) + ", ERROR]: ";
#else
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_RED);
    string error_message = GetTimeStamp() + "]: ";
#endif
    error_message += msg;
    error_message.append("\n");
    if (line != 0)
    {
        error_message += "[FILE]: ";
        error_message += file;
        error_message += "\n[LINE]: ";
        error_message += std::to_string(line) + "\n";
    }
#if !defined(_WIN32)
    error_message += to_string(line) + "\n\033[0m";
    cout << error_message;
#else
    cout << error_message;
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_WHITE);
#endif
    error_message = error_message.substr(prefixSize);
    error_message = error_message.substr(0, error_message.find_last_of("\a"));
    if (InternalFileLoggingEnabled)
    {
        fwrite(error_message.c_str(), 1, error_message.size(), InternalFileHandle);
        fflush(InternalFileHandle);
    }
    bool ok_button = false;
    if (show_message_box)
        ok_button = Utils::Message("Engine Error!", error_message.c_str(), Utils::MBox::ERR);

    return ok_button;
}

void log_fatal(int exit_code, const char *msg, const char *file, int line)
{
    PROFILE_FUNCTION();
    int prefixSize = 0;
#if !defined(_WIN32)
    string error_message = GetTimeStamp(POSIX_COLOR_BRIGHT_RED, POSIX_BGCOLOR_BLACK, prefixSize) + ", ERROR]: ";
#else
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_RED);
    string error_message = GetTimeStamp() + "]: ";
#endif
    error_message += msg;
    if (line != 0)
    {
        error_message += "\n[FILE]: ";
        error_message += file;
        error_message += "\n[LINE]: ";
        error_message += std::to_string(line) + "\n";
    }
#if !defined(_WIN32)
    error_message += to_string(line) + "\n\033[0m";
    cout << error_message;
#else
    cout << error_message;
    SetConsoleTextAttribute(ConsoleHandle, WIN32FG_WHITE);
#endif
    error_message = error_message.substr(prefixSize);
    error_message = error_message.substr(0, error_message.find_last_of("\a"));
    if (InternalFileLoggingEnabled)
    {
        fwrite(error_message.c_str(), 1, error_message.size(), InternalFileHandle);
        fflush(InternalFileHandle);
    }

    Utils::Message("FATAL Engine Error!", error_message.c_str(), Utils::MBox::ERR);
#if defined(_WIN32)
    ExitProcess(exit_code);
#else
    exit(exit_code);
#endif
}

#if !defined(_WIN32) && !defined(NO_LOGGER)
string GetColorPrefix(POSIX_COLORS ForegroundColor, POSIX_COLORS BackgroundColor)
{
    int foregroundColor = POSIX_COLOR_WHITE;
    int backgroundColor = POSIX_BGCOLOR_BLACK;

    if (ForegroundColor == POSIX_COLOR_BLACK)
    {
        foregroundColor = POSIX_COLOR_BLACK;
    }
    else if (ForegroundColor == POSIX_COLOR_RED)
    {
        foregroundColor = POSIX_COLOR_RED;
    }
    else if (ForegroundColor == POSIX_COLOR_GREEN)
    {
        foregroundColor = POSIX_COLOR_GREEN;
    }
    else if (ForegroundColor == POSIX_COLOR_YELLOW)
    {
        foregroundColor = POSIX_COLOR_YELLOW;
    }
    else if (ForegroundColor == POSIX_COLOR_BLUE)
    {
        foregroundColor = POSIX_COLOR_BLUE;
    }
    else if (ForegroundColor == POSIX_COLOR_MAGENTA)
    {
        foregroundColor = POSIX_COLOR_MAGENTA;
    }
    else if (ForegroundColor == POSIX_COLOR_CYAN)
    {
        foregroundColor = POSIX_COLOR_CYAN;
    }
    else if (ForegroundColor == POSIX_COLOR_WHITE)
    {
        foregroundColor = POSIX_COLOR_WHITE;
    }
    else if (ForegroundColor == POSIX_COLOR_BRIGHT_BLACK)
    {
        foregroundColor = POSIX_COLOR_BRIGHT_BLACK;
    }
    else if (ForegroundColor == POSIX_COLOR_BRIGHT_RED)
    {
        foregroundColor = POSIX_COLOR_BRIGHT_RED;
    }
    else if (ForegroundColor == POSIX_COLOR_BRIGHT_GREEN)
    {
        foregroundColor = POSIX_COLOR_BRIGHT_GREEN;
    }
    else if (ForegroundColor == POSIX_COLOR_BRIGHT_YELLOW)
    {
        foregroundColor = POSIX_COLOR_BRIGHT_YELLOW;
    }
    else if (ForegroundColor == POSIX_COLOR_BRIGHT_BLUE)
    {
        foregroundColor = POSIX_COLOR_BRIGHT_BLUE;
    }
    else if (ForegroundColor == POSIX_COLOR_BRIGHT_MAGENTA)
    {
        foregroundColor = POSIX_COLOR_BRIGHT_MAGENTA;
    }
    else if (ForegroundColor == POSIX_COLOR_BRIGHT_CYAN)
    {
        foregroundColor = POSIX_COLOR_BRIGHT_CYAN;
    }
    else if (ForegroundColor == POSIX_COLOR_BRIGHT_WHITE)
    {
        foregroundColor = POSIX_COLOR_BRIGHT_WHITE;
    }

    if (BackgroundColor == POSIX_BGCOLOR_BLACK)
    {
        backgroundColor = POSIX_BGCOLOR_BLACK;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_RED)
    {
        backgroundColor = POSIX_BGCOLOR_RED;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_GREEN)
    {
        backgroundColor = POSIX_BGCOLOR_GREEN;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_YELLOW)
    {
        backgroundColor = POSIX_BGCOLOR_YELLOW;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BLUE)
    {
        backgroundColor = POSIX_BGCOLOR_BLUE;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_MAGENTA)
    {
        backgroundColor = POSIX_BGCOLOR_MAGENTA;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_CYAN)
    {
        backgroundColor = POSIX_BGCOLOR_CYAN;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_WHITE)
    {
        backgroundColor = POSIX_BGCOLOR_WHITE;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BRIGHT_BLACK)
    {
        backgroundColor = POSIX_BGCOLOR_BRIGHT_BLACK;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BRIGHT_RED)
    {
        backgroundColor = POSIX_BGCOLOR_BRIGHT_RED;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BRIGHT_GREEN)
    {
        backgroundColor = POSIX_BGCOLOR_BRIGHT_GREEN;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BRIGHT_YELLOW)
    {
        backgroundColor = POSIX_BGCOLOR_BRIGHT_YELLOW;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BRIGHT_BLUE)
    {
        backgroundColor = POSIX_BGCOLOR_BRIGHT_BLUE;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BRIGHT_MAGENTA)
    {
        backgroundColor = POSIX_BGCOLOR_BRIGHT_MAGENTA;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BRIGHT_CYAN)
    {
        backgroundColor = POSIX_BGCOLOR_BRIGHT_CYAN;
    }
    else if (BackgroundColor == POSIX_BGCOLOR_BRIGHT_WHITE)
    {
        backgroundColor = POSIX_BGCOLOR_BRIGHT_WHITE;
    }

    string prefix = "\033[1;" + to_string(foregroundColor) + ";" + to_string(backgroundColor) + "m";

    return prefix;
}
#endif
