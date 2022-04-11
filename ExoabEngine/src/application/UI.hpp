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
	extern int Frequency[3];
	extern int Octave[3];
	extern bool RegenerateNoiseMap;
	extern int ActiveNoiseMap;
	extern float Contribution[3];
	extern bool SaveTerrain;
	extern bool ShowStatistics;
	extern float Exposure;

	void Initalize(void* context, void* gfx);
	void RenderUI();
	void UpdateNoiseMap(ITexture2 noise1, ITexture2 noise2, ITexture2 noise3, VkSampler sampler);
}
