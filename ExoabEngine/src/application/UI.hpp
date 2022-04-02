#pragma once
#include <memory/Texture2.hpp>
#include <glm/glm.hpp>

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
	extern int CurrentOutputBuffer;
	extern bool VSync;
	extern glm::vec3 CameraPosition;
	extern glm::vec3 LightPosition;
	extern bool ReloadShaders;
	extern bool ClearShaderCache;
	extern int BloomDownsampleMip;
	extern float Frequency;
	extern int Octave;
	extern bool RegenerateNoiseMap;

	void Initalize(void* context, void* gfx);
	void RenderUI();
	void UpdateNoiseMap(ITexture2 texture, VkSampler sampler);
}
