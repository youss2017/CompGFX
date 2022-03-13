#version 450 core

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in vec3 Weights;

layout (location = 0) out vec4 FragColor;

void main() {
	FragColor = vec4(Weights, 1.0);
}	