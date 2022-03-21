#version 450 core
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec4 inNormal;
layout (location = 2) in ivec4 inTextureIDs;
layout (location = 3) in vec4 inTextureWeights;
layout (location = 4) in vec2 inTexCoords;

layout (constant_id = 0) const float scale_x = 1.0;
layout (constant_id = 1) const float scale_y = 1.0;
layout (constant_id = 2) const float scale_z = 1.0;

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

layout (location = 0) out vec3 Normal;
layout (location = 1) out ivec3 TextureIDs;
layout (location = 2) out vec3 TextureWeights;
layout (location = 3) out vec2 TexCoord;
layout (location = 4) out vec4 LightSpacePos;

void main()
{
    Normal = normalize(inNormal.xyz);
    TextureIDs = inTextureIDs.xyz;
    TextureWeights = inTextureWeights.xyz;
    vec4 scale = vec4(scale_x, scale_y, scale_z, 1.0);
    TexCoord = inTexCoords.xy * scale.xy;
    LightSpacePos = u_LightSpace * (inPosition * scale);
    gl_Position = u_ProjView * (scale * inPosition);
}
