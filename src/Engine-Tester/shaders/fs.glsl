#version 450 core

layout (location = 0) in vec3 normal;
layout (location = 0) out vec3 outFrag;

const vec3 lightDirection = vec3(0.0, -1.0, 0.0);

void main() {
	outFrag = -dot(normal, lightDirection) * vec3(1.0) + vec3(0.35);
}