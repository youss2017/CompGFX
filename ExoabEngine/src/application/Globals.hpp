#pragma once
#include <backend/VkGraphicsCard.hpp>
#include <../window/PlatformWindow.hpp>
#include <Graphics.hpp>
#include <glm/mat4x4.hpp>
#include "../mesh/geometry.hpp"

namespace Global {
	extern bool Quit;
	extern vk::VkContext Context;
	extern PlatformWindow* Window;
	extern ConfigurationSettings Configuration;
	extern double TimeFromStart;
	extern double Time;
	extern double FrameRate;
	extern bool UpdateUIInfo;
	extern glm::mat4 Projection;
	extern bool RenderDOC;
	extern std::vector<Mesh::Geometry> Geomtry;
	extern VkSampler DefaultSampler;
}
