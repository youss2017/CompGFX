#pragma once

namespace UI
{
	extern bool StateChanged;
	extern bool ShowDepthBuffer;
	extern bool ShowWireframe;
	extern double FrameRate;
	extern float C_x, C_y, C_z;
	void Initalize(void* context, void* gfx);
	void RenderUI();
}
