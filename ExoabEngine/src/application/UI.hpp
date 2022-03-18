#pragma once

namespace UI
{
	extern bool StateChanged;
	extern bool ShowDepthBuffer;
	extern bool ShowWireframe;
	extern bool LockFrustrum;
	extern double FrameRate;
	extern double FrameTime;
	extern double FrustrumCullingTime;
	extern unsigned long long  FrustrumInvocations;
	extern double InputDrawCount;
	extern double OutputDrawCount;
	extern double GPUPassTime;
	extern float C_x, C_y, C_z;
	void Initalize(void* context, void* gfx);
	void RenderUI();
}
