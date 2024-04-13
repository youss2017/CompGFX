#version 450

layout (location = 0) in vec3 color_or_uv;
layout (location = 1) in float opacity;

layout (location = 0) out vec4 FragColor;

void main() {

	FragColor = vec4(color_or_uv, opacity + 1);
}