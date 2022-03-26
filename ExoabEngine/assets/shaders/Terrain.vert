#version 450 core
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inBiTangent;
layout (location = 4) in ivec3 inTextureIDs;
layout (location = 5) in vec3 inTextureWeights;
layout (location = 6) in vec2 inTexCoords;

layout (scalar, binding = 0) uniform GlobalDataUBO
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
    mat3 u_NormalModel; // transpose(inverse(u_Model)) (no offsets)
};

layout (location = 0) out ivec3 TextureIDs;
layout (location = 1) out vec3 TextureWeights;
layout (location = 2) out vec2 TexCoord;
layout (location = 3) out vec4 LightSpacePos;
layout (location = 4) out mat3 TBN;

void main()
{
    vec3 T = normalize(u_NormalModel * inTangent);
    vec3 B = normalize(u_NormalModel * inBiTangent);
    vec3 N = normalize(u_NormalModel * inNormal);
    TBN = mat3(T, B, N);

    TextureIDs = inTextureIDs;
    TextureWeights = inTextureWeights;

    TexCoord = inTexCoords;
    vec4 position = u_Model * vec4(inPosition, 1.0);
    
    LightSpacePos = u_LightSpace * position;
    gl_Position = u_ProjView * position;
}
