#version 450

layout (binding = 0) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 FragColor;

layout (push_constant) uniform pushblock {
    layout(offset = 64) uint u_Lod;
};

void main() 
{
	FragColor = textureLod(samplerCubeMap, inUVW, u_Lod);
}