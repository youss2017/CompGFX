#version 450 core

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex
{
    f16vec3 inPosition;
    u8vec3 inNormal;
    u8vec3 inTextureIDs;
    u8vec3 inTextureWeights;
    f16vec2 inTexCoords;
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
#define decompress_normal(normal) (vec3(int(normal.x), int(normal.y), int(normal.z)) / 127.0 - 1.0)

void main()
{
    Normal = normalize(mat3(u_NormalModel) * decompress_normal(vertex.inNormal));
    TextureIDs = uvec3(vertex.inTextureIDs.x, vertex.inTextureIDs.y, vertex.inTextureIDs.z);
    TextureWeights = vec3(uint(vertex.inTextureWeights.x), uint(vertex.inTextureWeights.y), uint(vertex.inTextureWeights.z)) / 255.0;
    TexCoord = vec2(float(vertex.inTexCoords.x), float(vertex.inTexCoords.y));
    gl_Position = u_Projection * u_View * u_Model * vec4(vertex.inPosition, 1.0);
}
