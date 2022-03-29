#pragma once
#include <stdint.h>

namespace Utils {
	void perlin(uint32_t width, uint32_t height, uint32_t seed, float frequency, float octaves, uint8_t* output_buffer);
}