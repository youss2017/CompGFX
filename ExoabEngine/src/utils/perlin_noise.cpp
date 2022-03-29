#include "perlin_noise.hpp"
#include <time.h>
#include <math.h>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include "common.hpp"
#include "PerlinNoise.hpp"

using namespace glm;

namespace Utils {
	void perlin(uint32_t width, uint32_t height, uint32_t _seed, float frequency, float octaves, uint8_t* output_buffer) {
		uint32_t w = width;
		uint32_t h = height;
		
		siv::PerlinNoise::seed_type seed = _seed;
		siv::PerlinNoise perlin{ seed };
		double fx = frequency / float(width);
		double fy = frequency / float(height);
#if 0
		uint8_t* image_buffer = new uint8_t[w * h];
#endif
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				double noise = perlin.octave2D_01((x * fx), (y * fy), octaves);
#if 0
				image_buffer[y * w + x] = uint8_t(noise * 255.0f);
#else
				output_buffer[y * w + x] = uint8_t(noise * 255.0f);
#endif
			}
		}
#if 0
		stbi_write_png("perlin.png", w, h, 1, image_buffer, w);
		system("start perlin.png");
		delete[] image_buffer;
		exit(0);
#endif
	}
}