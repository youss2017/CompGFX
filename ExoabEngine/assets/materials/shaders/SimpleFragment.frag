#version 450 core

layout (location = 0) in vec3 Color;
layout (location = 0) out vec4 FragColor;

void main() {
	FragColor = vec4(Color.rgb, 0.0);
}