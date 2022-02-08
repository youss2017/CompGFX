#pragma once
#include "../Graphics.hpp"
#include <vector>

class FramebufferReserve
{
public:
	FramebufferReserve(GraphicsContext context, ConfigurationSettings& cfg, const char* reserve_path);
	~FramebufferReserve();

public:
	std::vector<IGPUTexture2D> m_attachments;

};