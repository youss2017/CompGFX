#version 450 core
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_16bit_storage : require

struct TerrainVertex {
    vec3 inPosition;
    f16vec3 inNormal;
    f16vec3 inTangent;
    f16vec3 inBiTangent;
    i16vec3 inTextureIDs;
    f16vec3 inTextureWeights;
    f16vec2 inTexCoords;
};

layout (scalar, binding = 0) readonly buffer VerticesSSBO {
    TerrainVertex[] u_Vertices;
};

layout (binding = 1) uniform MVBuffer {
	mat4 u_ShadowTOffset;
	mat4 u_ProjView;
};

void main() {
	gl_Position = u_ProjView * u_ShadowTOffset * vec4(u_Vertices[gl_VertexIndex].inPosition, 1.0);
}