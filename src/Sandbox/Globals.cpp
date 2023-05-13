#include "Globals.hpp"

namespace shared::globals {

	std::vector<std::string> ProgramArgs;
	std::map<std::string, std::string> EnvironmentVars;

	egx::ref<egx::PlatformWindow> Window;
	egx::ref<egx::VulkanCoreInterface> CoreInterface;
	egx::EngineCore* Engine;
	egx::VulkanSwapchain* Swapchain;
	std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> FrameStartPoint;
	double DeltaTime = 0.0;
	double AccumulatedTime = 0.0;

}