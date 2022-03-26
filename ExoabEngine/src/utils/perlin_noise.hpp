#pragma once
#include <stdint.h>

void perlin(uint32_t width, uint32_t height, float offset, uint8_t* output_buffer, uint32_t seed = 0);
