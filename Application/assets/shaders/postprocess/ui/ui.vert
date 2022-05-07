#version 450 core

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec3 Color;
layout (location = 1) out vec2 TexCoord;

layout (push_constant) uniform pushblock {
	vec3 u_Color;
};

void main() {
	Color = u_Color;
	TexCoord = inTexCoord;
	gl_Position = vec4(inPosition, 0.0, 1.0);
}