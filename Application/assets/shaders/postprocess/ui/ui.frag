#version 450 core

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 TexCoord;

layout (binding = 0) uniform sampler2D UITexture[2];
layout (push_constant) uniform pushblock {
	layout (offset = 16) int u_UsingTexture;
	int u_TextureID;
};

layout (location = 0) out vec4 FragColor;

void main() {
	if(u_UsingTexture == 1) {
		FragColor = texture(UITexture[u_TextureID], TexCoord);
	} else {
		FragColor = vec4(Color, 1.0);
	}
}