#version 450 core
layout (location = 0) in vec3 Pos;
layout (location = 1) in vec3 Normal;
layout (location = 3) in vec3 Bitangent;
layout (location = 4) in vec3 Antibitangent;
layout (location = 2) in vec2 TU;

layout (push_constant) uniform pushblock {
	mat4 Projection;
	float iTime;
	float iDeltaTime;
};

layout (binding = 0) buffer Material {
	vec4 Color;
	vec4 data;
	vec4 data2;
};

layout (set = 1, binding = 0) uniform mvp {
	mat4 view;
	mat4 model;
};

struct Mydata {
//	float x;
//	int y;
//	int z;
	mat4 custom;
	mat3 data;
};

layout (set = 1, binding = 1) buffer Material2 {
	Mydata MatData[];
};

void main() {}
