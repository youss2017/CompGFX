#version 450 core

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 TexCoord;

layout (binding = 0) uniform sampler2D UITexture;
layout (push_constant) uniform pushblock {
	layout (offset = 16) int u_UsingTexture;
};

layout (location = 0) out vec4 FragColor;

void main() {
	if(u_UsingTexture == 1) {
		FragColor = texture(UITexture, TexCoord);
	} else {
		FragColor = vec4(Color, 1.0);
	}
}