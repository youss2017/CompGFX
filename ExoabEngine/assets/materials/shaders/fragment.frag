#version 450 core

layout (set = 1, binding = 0) uniform sampler2D tex0[1];

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec2 TexCoord;

const vec3 LightDir = vec3(0.0, -0.5, 0.5);

layout (location = 0) out vec4 FragColor;

float LinearizeDepth(float depth, float near, float far) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {
	float dp = clamp(dot(LightDir, Normal), 0.08, 1.0);
	FragColor = (texture(tex0[0], TexCoord) * dp);
	FragColor = vec4(cos(TexCoord.x), sin(TexCoord.y), 0.0, 0.0) * dp;
}	