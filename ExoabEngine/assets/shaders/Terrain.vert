#version 450 core

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex
{
    vec4 inPosition;
    vec4 inNormal;
    uvec4 inTextureIDs;
    uvec4 inTextureWeights;
    vec2 inTexCoords;
    int padding[2];
};

layout (set = 0, binding = 0) readonly buffer Vertices
{
    Vertex u_Vertices[];
};

layout (set = 0, binding = 1) uniform transform
{
    mat4 u_View;
    mat4 u_Model;
    mat4 u_NormalModel;
    mat4 u_Projection;
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out uvec3 TextureIDs;
layout (location = 2) out vec3 TextureWeights;
layout (location = 3) out vec2 TexCoord;

#define vertex u_Vertices[gl_VertexIndex]
void main()
{
    Normal = normalize(mat3(u_NormalModel) * vertex.inNormal.xyz);
    TextureIDs = vertex.inTextureIDs.xyz;
    TextureWeights = vertex.inTextureWeights.xyz;
    TexCoord = vertex.inTexCoords.xy;
    gl_Position = u_Projection * u_View * u_Model * vertex.inPosition;
}
