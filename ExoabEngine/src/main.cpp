#if defined(_WIN32)
#include <Windows.h>
#endif
#include <chrono>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
#include <timeapi.h>
#include <direct.h>
#include <Objbase.h>
#else
#include <unistd.h>
#endif

#include "utils/StringUtils.hpp"
#include "utils/Profiling.hpp"
#include "Exoab.hpp"
#include "physics/Physics.hpp"
#include <csignal>

#ifdef _WIN32
static void PrepareWin32(int argc, char** argv);
#else
static void PreparePOSIX(int argc, char** argv);
#endif

/*
    (TODO)
        - Support Feedback Transform
        - https://github.com/zeux/meshoptimizer mesh optimizer
*/

static ConfigurationSettings LoadConfiguration();

int main(int argc, char** argv)
{
    PROFILE_BEGIN_SESSION("Startup", "Profiling-Startup.json");
#ifdef _WIN32
    PrepareWin32(argc, argv);
#else
    PreparePOSIX(argc, argv);
#endif
    log_configure(true, true);
    /* Load Game Settings from settings.cfg */
    ConfigurationSettings config = LoadConfiguration();
    if (!Physx_Initalize())
        log_fatal(0x5, "Could not initalize physx!", __FILE__, __LINE__);
    if(Exoab_Initalize(config) == false) {
        log_error("Could not initalize Game");
    }
    PROFILE_END_SESSION();

    PROFILE_BEGIN_SESSION("Runtime", "Profiling-Runtime.json");
    double dElapsedTime = 0;
    double dTimeFromStart = 0.0;
    auto start = std::chrono::high_resolution_clock::now();
    double FrameRate = 0.0;
    double Checkpoint = 0;
    Tristate state = Tristate::Enabled();
    while (!state.disabled())
    {
        state = Exoab_Update(dTimeFromStart, dElapsedTime);
        if (state.enabled())
            Exoab_Render(dTimeFromStart, dElapsedTime, FrameRate);
        auto end = std::chrono::high_resolution_clock::now();
        dElapsedTime = (end - start).count();
        start = end;
        dElapsedTime /= 1000'000'000;
        dTimeFromStart += dElapsedTime;
        if (dTimeFromStart - Checkpoint >= 5.4e-1) {
            Checkpoint = dTimeFromStart;
            FrameRate = 1.0 / dElapsedTime;
        }
    }
    PROFILE_END_SESSION();
    PROFILE_BEGIN_SESSION("Cleanup", "Profiling-Cleanup.json");
    Exoab_CleanUp();
    Physx_Destroy();
    PROFILE_END_SESSION();
    return 0;
}

static ConfigurationSettings LoadConfiguration()
{
    FILE *settingsIO = fopen("assets/settings.cfg", "r");
    ConfigurationSettings config;
    if (settingsIO)
    {
        std::vector<std::string> settingscfgLines;
        while (!feof(settingsIO))
            settingscfgLines.push_back(Utils::StrUpperCase(Utils::StrTrim(Utils::ReadLine(settingsIO))));
        fclose(settingsIO);
        for (auto &line : settingscfgLines)
        {
            if (line[0] != '[')
                continue;
            if (Utils::StrStartsWith(line, "[API]"))
            {
                config.ApiType = (Utils::StrContains(line, "VULKAN")) ? EngineAPIType::Vulkan : EngineAPIType::OpenGL;
            }
            else if (Utils::StrStartsWith(line, "[WIDTH]"))
            {
                bool fail;
                int width = Utils::StrGetFirstNumber(line, fail);
                if (!fail)
                    config.ResolutionWidth = width;
            }
            else if (Utils::StrStartsWith(line, "[HEIGHT]"))
            {
                bool fail;
                int height = Utils::StrGetFirstNumber(line, fail);
                if (!fail)
                    config.ResolutionHeight = height;
            }
            else if (Utils::StrStartsWith(line, "[FULLSCREEN]"))
            {
                if (Utils::StrContains(line, "TRUE"))
                    config.Fullscreen = true;
                else
                    config.Fullscreen = false;
            }
            else if (Utils::StrStartsWith(line, "[MSAA]")) {
                bool fail;
                int SampleCount = Utils::StrGetFirstNumber(line, fail);
                if(fail)
                {logalert("Invalid MSAA Count from settings.cfg"); SampleCount = 1;}
                // Check if it is a vaild sample count
                SampleCount = std::max(std::pow(2.0, (int)std::round(std::log2(SampleCount))), 1.0);
                config.MSAASamples = (TextureSamples)SampleCount;
            } else {
                printf("[settings.cfg]: Unknown '%s'\n", line.c_str());
            }
        }
    } else {
        logalert("Could not load settings.cfg --- Using default settings instead.");
    }
    return config;
}

#ifndef _WIN32
static void PreparePOSIX(int argc, char** argv)
{
    char buf[512];
    getcwd(buf, sizeof(buf));
    printf("Working Directory: %s\n", buf);
}
#else
void PrepareDLLs();
static void PrepareWin32(int argc, char** argv)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    PrepareDLLs();
#if 0
    if(timeBeginPeriod(1) == TIMERR_NOERROR)
        log_info("Succesfully increased windows timer resolution to 1 ms");
    else {
        log_warning("Failed increased windows timer resolution to 1 ms");
        timeBeginPeriod(5);
    }
#endif
}
#endif
