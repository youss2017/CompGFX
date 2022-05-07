#version 450 core
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require

struct TerrainVertex {
    f16vec3 inPosition;
    f16vec3 inNormal;
    f16vec3 inTangent;
    u8vec3 inTextureIDs;
    f16vec3 inTextureWeights;
    f16vec2 inTexCoords;
};

layout (scalar, binding = 0) readonly buffer VerticesSSBO {
    TerrainVertex[] u_Vertices;
};

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

layout (scalar, push_constant) uniform pushblock {
    mat4 u_Model;
    mat3 u_NormalModel; // mat3(transpose(inverse(u_Model)))
};

layout (location = 0) out flat uvec3 TextureIDs;
layout (location = 1) out vec3 TextureWeights;
layout (location = 2) out vec2 TexCoord;
layout (location = 3) out vec4 LightSpacePos;
layout (location = 4) out vec3 FragPos;
layout (location = 5) out mat3 TBN;

#define vertex u_Vertices[gl_VertexIndex]

void main()
{
    vec3 normal =  vec3(vertex.inNormal);
    vec3 tangent = vec3(vertex.inTangent);
    vec3 bitangent = cross(normal, tangent);
    vec3 T = normalize(u_NormalModel * tangent);
    vec3 B = normalize(u_NormalModel * bitangent);
    vec3 N = normalize(u_NormalModel * normal);
    TBN = mat3(T, B, N);

    TextureIDs = uvec3(vertex.inTextureIDs);
    TextureWeights = vec3(vertex.inTextureWeights);

    TexCoord = vec2(vertex.inTexCoords);
    vec4 position = u_Model * vec4(vec3(vertex.inPosition), 1.0);
    
    LightSpacePos = u_LightSpace * position;
    FragPos = position.xyz;
    gl_Position = u_ProjView * position;
}
