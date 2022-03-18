#version 450 core

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable

layout (scalar, binding = 1) uniform GlobalDataUBO
{
	float u_DeltaTime;
	float u_TimeFromStart;
	vec4 u_CameraForward;
	mat4 u_View;
	mat4 u_Projection;
	mat4 u_ProjView;
};

layout (set = 1, binding = 0) uniform sampler2D textures[];

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in flat uint TexIndex;

const vec3 LightDir = normalize(vec3(0.0, 0.5, -0.5));

layout (location = 0) out vec4 FragColor;

float LinearizeDepth(float depth, float near, float far) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {
	float dp = clamp(dot(u_CameraForward.xyz, Normal), 0.2, 1.0);
	FragColor = texture(textures[TexIndex], TexCoord * sin(u_TimeFromStart)) * dp;
}	