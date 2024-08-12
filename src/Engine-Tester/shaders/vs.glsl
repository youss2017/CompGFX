#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec3 normal;

const vec3 lightDirection = vec3(0.0, -1.0, 0.0);

layout (push_constant) uniform pb {
	mat4 transform;
	mat4 ortho;
};

void main() {;
	normal = normalize((transform * vec4(normal, 1.0)).xyz);
	gl_Position = (ortho * transform * vec4(inPos, 1.0)) * vec4(1.0, -1.0, 1.0, 1.0);
}