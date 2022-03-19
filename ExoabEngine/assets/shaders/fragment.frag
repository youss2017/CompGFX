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

float LinearizeDepth(float depth, float near, float far) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    return shadow;
} 

void main() {
	float dp = clamp(dot(u_CameraForward.xyz, Normal), 0.2, 1.0);
	float shadow = ShadowCalculation(LightSpacePos);
	FragColor = texture(textures[TexIndex], TexCoord * sin(u_TimeFromStart)) * dp * (1.0 - shadow);
}	