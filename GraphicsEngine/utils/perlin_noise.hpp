#pragma once
#include <stdint.h>

#ifndef GRAPHICS_API
#ifdef BUILD_GRAPHICS_DLL
#define GRAPHICS_API _declspec(dllexport)
#else
#define GRAPHICS_API _declspec(dllimport)
#endif
#endif

namespace Utils {
	void GRAPHICS_API perlin(uint32_t width, uint32_t height, uint32_t seed, float frequency, float octaves, float* output_buffer);
}