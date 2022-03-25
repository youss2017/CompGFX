#pragma once

namespace UI
{
	extern bool StateChanged;
	extern bool ShowWireframe;
	extern bool LockFrustrum;
	extern double FrameRate;
	extern double FrameTime;
	extern double FrustrumCullingTime;
	extern double BloomPassTime;
	extern double ShadowPassTime;
	extern double DebugPassTime;
	extern double NextFrameTime;
	extern unsigned long long  FrustrumInvocations;
	extern double GeometryPassTime;
	extern unsigned long long VertexInvocations;
	extern unsigned long long FragmentInvocations;
	extern int CubemapLOD;
	extern int CubemapLODMax;
	extern float C_x, C_y, C_z;
	extern int CurrentOutputBuffer;
	extern bool VSync;
	extern float L_x, L_y, L_z;
	extern bool ReloadShaders;
	extern bool ClearShaderCache;
	extern int BloomDownsampleMip;

	void Initalize(void* context, void* gfx);
	void RenderUI();
}
