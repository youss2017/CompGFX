#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in uvec3 inTextureIDs;
layout (location = 3) in vec3 inTextureWeights;
layout (location = 4) in vec2 inTexCoords;

layout (set = 0, binding = 0) uniform transform
{
    mat4 view;
    mat4 model;
    mat4 inverse_model;
    mat4 projection;
} t;

layout (location = 0) out vec3 Normal;
layout (location = 1) out uvec3 TextureIDs;
layout (location = 2) out vec3 TextureWeights;
layout (location = 3) out vec2 TexCoord;

void main()
{
    Normal = normalize(mat3(t.inverse_model) * inNormal);
    TextureIDs = inTextureIDs;
    TextureWeights = inTextureWeights;
    TexCoord = inTexCoords;

    gl_Position = t.projection * t.view * t.model * vec4(inPosition, 1.0);
}