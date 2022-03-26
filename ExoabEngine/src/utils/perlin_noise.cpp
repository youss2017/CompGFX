#include "perlin_noise.hpp"
#include <time.h>
#include <math.h>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include "common.hpp"
using namespace glm;

static float N21(vec2 p, float iTime) {
	vec2 fp = fract(p);
	srand(fp.x+iTime);
	float a = rand();
	srand(fp.y+ iTime);
	float b = rand();
	float c = rand();
	//return fract(sin(p.x * 100. + p.y * 6574.) * 5647.);
	return fract(sin(p.x * a/(1000.0) + p.y * b) * b/(5.0));
}

static float SmoothNoise(vec2 uv, float iTime) {
	vec2 lv = fract(uv);
	vec2 id = floor(uv);

	lv = lv * lv * (3.f - 2.f * lv);

	float bl = N21(id, iTime);
	float br = N21(id + vec2(1, 0), iTime);
	float b = mix(bl, br, lv.x);

	float tl = N21(id + vec2(0, 1), iTime);
	float tr = N21(id + vec2(1, 1), iTime);
	float t = mix(tl, tr, lv.x);

	return mix(b, t, lv.y);
}

static float shader(vec2 uv, float iTime) {

	float c = SmoothNoise(uv * 4.0f, iTime);
	c += SmoothNoise(uv * 8.2f, iTime) * .5;
	c += SmoothNoise(uv * 16.7f, iTime) * .25;
	c += SmoothNoise(uv * 32.4f, iTime) * .125;
	c += SmoothNoise(uv * 64.5f, iTime) * .0625;

	c /= 2.0;

	return c;
}

void perlin(uint32_t width, uint32_t height, float offset, uint8_t* output_buffer, uint32_t seed) {
	uint32_t w = width;
	uint32_t h = height;
	float iTime = (seed == 0) ? float(time(NULL)) + offset : seed + offset;
	for (uint32_t y = 0; y < h; y++) {
		for (uint32_t x = 0; x < w; x++) {
			output_buffer[y * w + x] = uint8_t(shader(vec2(x, y) / vec2(w, h), iTime) * 255.0f);
		}
	}
}
