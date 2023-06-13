#pragma once
#include <vector>
#include <glm/glm.hpp>
using namespace glm;
using namespace std;

inline const int width = 1280;
inline const int height = 720;
inline const int block_size = 40;
inline bool wrap_pixel_mode = true;
vector<uint32_t>* back_buffer = nullptr;
vector<uint8_t> depth_buffer = vector<uint8_t>(width * height);

__forceinline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
	return (r << 16) | (g << 8) | (b);
}

__forceinline uint32_t rgb(fvec3 color) {
	return rgb(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f);
}

__forceinline uint32_t rgb(ivec3 color) {
	return rgb(color.r, color.g, color.b);
}

__forceinline fvec3 from_rgbf(int color) {
	return vec3((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) / 255.0f;
}

__forceinline ivec3 from_rgb(int color) {
	return ivec3((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
}

__forceinline void draw_pixel(int x, int y, int color, uint8_t z_index) {
	if (x >= 0 && y >= 0) {
		if (!wrap_pixel_mode) {
			if (x >= width || y >= height)
				return;
		}
		int index = (y % height) * width + (x % width);
		if (depth_buffer[index] <= z_index) {
			back_buffer->operator[](index) = color;
			depth_buffer[index] = z_index;
		}
	}
}

void fill_rect(int x, int y, int w, int h, int color, uint8_t z_index) {
	for (int ypos = y; ypos < y + h; ypos++) {
		for (int xpos = x; xpos < x + w; xpos++) {
			draw_pixel(xpos, ypos, color, z_index);
		}
	}
}

void fill_circle(int x, int y, int r, int color, uint8_t z_index) {
	for (int ypos = y - r; ypos < y + r; ypos++) {
		int y_at_origin = ypos - y;
		int det = (r * r) - y_at_origin * y_at_origin;
		if (det < 0)
			continue;
		int start_x = -sqrtf(det);
		int end_x = -start_x;
		for (int xpos = start_x; xpos <= end_x; xpos++) {
			draw_pixel(xpos + x, ypos, color, z_index);
		}
	}
}

void draw_circle(int x, int y, int r, int thickness, int color, uint8_t z_index) {
	thickness = glm::clamp(thickness, 1, r);
	for (int ypos = y - r; ypos < y + r; ypos++) {
		int y_at_origin = ypos - y;
		int det = (r * r) - y_at_origin * y_at_origin;
		if (det < 0)
			continue;
		int start_x = -sqrtf(det);
		int end_x = -start_x;
		for (int xpos = start_x; xpos <= (start_x + thickness) && xpos <= end_x; xpos++) {
			draw_pixel(xpos + x, ypos, color, z_index);
		}
		for (int xpos = glm::max(end_x - thickness, start_x); xpos <= end_x; xpos++) {
			draw_pixel(xpos + x, ypos, color, z_index);
		}
	}
}

void draw_sprite(int x, int y, int w, int h, uint8_t z_index) {
	for (int ypos = y; ypos < y + h; ypos++) {
		for (int xpos = x; xpos < x + w; xpos++) {
			draw_pixel(xpos, ypos, 0, z_index);
		}
	}
}

__forceinline void clear(int color, uint8_t z_index) {
	memset(back_buffer->data(), color, sizeof(uint32_t) * back_buffer->size());
	memset(depth_buffer.data(), z_index, sizeof(uint8_t) * depth_buffer.size());
}

__forceinline int rand_color() {
	return rgb(rand() % 255, rand() % 255, rand() % 255);
}

__forceinline ivec2 block_to_pixel(int block_x, int block_y) {
	return { block_x * block_size, block_y * block_size };
}

__forceinline ivec2 block_to_pixel(ivec2 block_position) {
	return block_position * block_size;
}

__forceinline void fill_rect(ivec2 position, ivec2 size, int color, uint8_t z_index) {
	fill_rect(position.x, position.y, size.x, size.y, color, z_index);
}

__forceinline void fill_circle(ivec2 position, int r, int color, uint8_t z_index) {
	fill_circle(position.x, position.y, r, color, z_index);
}

__forceinline void draw_circle(ivec2 position, int r, int thickness, int color, uint8_t z_index) {
	draw_circle(position.x, position.y, r, thickness, color, z_index);
}

__forceinline void draw_player(ivec2 block_position, int color, uint8_t z_index) {
	int r = block_size / 2;
	fill_rect(block_position, ivec2(block_size), color, z_index);
	fill_circle(ivec2(block_position.x, block_position.y + block_size) + r, r, color, z_index);
}

__forceinline float rand_float() {
	return rand() / (float)RAND_MAX;
}

__forceinline float rand_unit_float() {
	return 2.0f * rand_float() - 1.0f;
}
