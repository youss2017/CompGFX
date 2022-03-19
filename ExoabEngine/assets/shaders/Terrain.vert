#version 450 core

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec4 inNormal;
layout (location = 2) in ivec4 inTextureIDs;
layout (location = 3) in vec4 inTextureWeights;
layout (location = 4) in vec2 inTexCoords;

layout (scalar, binding = 0) uniform GlobalDataUBO
{
    float u_DeltaTime;
    float u_TimeFromStart;
    vec4 u_CameraForward;
    mat4 u_View;
    mat4 u_Projection;
    mat4 u_ProjView;
    mat4 u_LightSpace;
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out ivec3 TextureIDs;
layout (location = 2) out vec3 TextureWeights;
layout (location = 3) out vec2 TexCoord;

void main()
{
    Normal = normalize(inNormal.xyz);
    TextureIDs = inTextureIDs.xyz;
    TextureWeights = inTextureWeights.xyz;
    TexCoord = inTexCoords.xy;
    gl_Position = u_ProjView * inPosition;
}
