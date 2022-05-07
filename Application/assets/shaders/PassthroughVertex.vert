#version 450

#extension GL_EXT_scalar_block_layout : enable

struct InputVertices {
    vec4 position;
    vec3 normal;
    vec2 texcoord;
    uint textureID;
};

layout (scalar, binding = 0) readonly buffer PassthroughVertices {
    InputVertices u_Vertices[];
};

layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texcoord;
layout (location = 2) out flat uint textureID;

void main() {
    gl_Position = u_Vertices[gl_VertexIndex].position;
    normal = u_Vertices[gl_VertexIndex].normal;
    texcoord = u_Vertices[gl_VertexIndex].texcoord;
    textureID = u_Vertices[gl_VertexIndex].textureID;
}