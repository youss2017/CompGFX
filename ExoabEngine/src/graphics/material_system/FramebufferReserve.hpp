#pragma once
#include "../Graphics.hpp"
#include <memory/Texture2.hpp>
#include <vector>

class FramebufferReserve
{
public:
	FramebufferReserve(GraphicsContext context, ConfigurationSettings& cfg, const char* reserve_path);
	~FramebufferReserve();

public:
	std::vector<ITexture2> m_attachments;

};