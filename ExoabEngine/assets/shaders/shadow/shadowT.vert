#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inBiTangent;
layout (location = 4) in ivec3 inTextureIDs;
layout (location = 5) in vec3 inTextureWeights;
layout (location = 6) in vec2 inTexCoords;

layout (binding = 0) uniform MVBuffer {
	mat4 u_ShadowTOffset;
	mat4 u_ProjView;
};

void main() {
	gl_Position = u_ProjView * u_ShadowTOffset * vec4(inPosition, 1.0);

}