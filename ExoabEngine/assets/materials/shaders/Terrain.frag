#version 450 core

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 Normal;
layout (location = 1) in flat uvec3 TextureIDs;
layout (location = 2) in vec3 TextureWeights;
layout (location = 3) in vec2 TexCoord;

// The first texture is the base texture
layout (set = 1, binding = 0) uniform sampler TexSampler;
layout (set = 1, binding = 1) uniform texture2D TerrainTextures[];

layout (push_constant) uniform pushblock
{
    vec3 LightDirection;
} LightInfo;

layout (location = 0) out vec4 FragColor;

void main()
{
	vec2 TiledTexCoord = TexCoord * 40.0;
    float BaseWeight = 1.0 - TextureWeights[0] + TextureWeights[1] + TextureWeights[2];
    vec4 BaseTexture = texture(sampler2D(TerrainTextures[0], TexSampler), TexCoord) * BaseWeight;
    vec4 Texture1 = texture(sampler2D(TerrainTextures[nonuniformEXT(TextureIDs.x)], TexSampler), TexCoord) * TextureWeights[TextureIDs.x];
    vec4 Texture2 = texture(sampler2D(TerrainTextures[nonuniformEXT(TextureIDs.y)], TexSampler), TexCoord) * TextureWeights[TextureIDs.y];
    vec4 Texture3 = texture(sampler2D(TerrainTextures[nonuniformEXT(TextureIDs.z)], TexSampler), TexCoord) * TextureWeights[TextureIDs.z];
    vec4 FinalTexture = BaseTexture + Texture1 + Texture2 + Texture3;
    float CosineBetweenLight = dot(Normal, LightInfo.LightDirection);

    // TODO: Support Normal Map and Specular Map
    FragColor = (FinalTexture - FinalTexture + texture(sampler2D(TerrainTextures[0], TexSampler), TiledTexCoord)) * CosineBetweenLight;
}