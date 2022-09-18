#include "perlin_noise.hpp"
#include <time.h>
#include <math.h>
#include <glm/glm.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <iostream>
#include "Profiling.hpp"
#include "PerlinNoise.hpp"

using namespace glm;

namespace Utils {
	void EGX_API perlin(uint32_t width, uint32_t height, uint32_t _seed, float frequency, float octaves, float* output_buffer) {
		PROFILE_FUNCTION();
		uint32_t w = width;
		uint32_t h = height;
		
		siv::PerlinNoise::seed_type seed = _seed;
		siv::PerlinNoise perlin{ seed };
		double fx = frequency / float(width);
		double fy = frequency / float(height);
#if 0
		uint8_t* imageBuffer = new uint8_t[width * height];
#endif
		for (uint32_t y = 0; y < h; y++) {
			for (uint32_t x = 0; x < w; x++) {
				double noise = perlin.octave2D_01((x * fx), (y * fy), octaves);
#if 0
				imageBuffer[y * w + x] = uint8_t(noise * 255.0);
#else
				output_buffer[y * w + x] = noise;
#endif
			}
		}
#if 0
		stbi_write_png("perlin.png", w, h, 1, imageBuffer, w);
		system("start perlin.png");
		exit(0);
#endif
	}
}