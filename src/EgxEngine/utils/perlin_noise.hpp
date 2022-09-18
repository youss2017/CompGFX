#pragma once
#include <stdint.h>

#undef EGX_API
#ifdef BUILD_GRAPHICS_DLL
#if BUILD_GRAPHICS_DLL == 1
#define EGX_API _declspec(dllexport)
#else
#define EGX_API _declspec(dllimport)
#endif
#else
#define EGX_API
#endif

namespace Utils {
	void EGX_API perlin(uint32_t width, uint32_t height, uint32_t seed, float frequency, float octaves, float* output_buffer);
}