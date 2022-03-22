#version 450 core

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable

layout (scalar, binding = 1) uniform GlobalDataUBO
{
	float u_DeltaTime;
	float u_TimeFromStart;
	vec4 u_LightDirection;
	mat4 u_View;
	mat4 u_Projection;
	mat4 u_ProjView;
	mat4 u_LightSpace;
};

layout (set = 1, binding = 0) uniform sampler2D textures[];
layout (set = 1, binding = 1) uniform sampler2D shadowMap;

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in flat uint TexIndex;
layout (location = 3) in vec4 LightSpacePos;

const vec3 LightDir = normalize(vec3(0.0, 0.5, -0.5));

layout (location = 0) out vec4 FragColor;

float ShadowCalculation()
{
    vec3 pos = LightSpacePos.xyz * 0.5 + 0.5;
    if(pos.z > 1.0) {
        pos.z = 1.0;
    }
    float depth = texture(shadowMap, pos.xy).r + 0.1;
    return depth < pos.z ? 1.0 : 0.0;
}

void main() {
	float dp = clamp(dot(u_LightDirection.xyz, Normal), 0.2, 1.0);
	FragColor = texture(textures[TexIndex], TexCoord) * dp * clamp((1.0 - ShadowCalculation()), 0.5, 1.0);
}	