#version 450 core
// This extension allows us to not specifiy the maximum textures,
// however this extension was release in 2018, and I do not know
// how well this extension is support it
//#extension GL_EXT_nonuniform_qualifier : require

// This can be changed later.
#define MAX_TERRAIN_TEXTURES 1

layout (location = 0) in vec3 Normal;
layout (location = 1) in flat uvec3 TextureIDs;
layout (location = 2) in vec3 TextureWeights;
layout (location = 3) in vec2 TexCoord;

// The first texture is the base texture
layout (set = 1, binding = 0) uniform sampler2D TerrainTextures[MAX_TERRAIN_TEXTURES];

layout (push_constant) uniform pushblock
{
    vec3 LightDirection;
} LightInfo;

layout (location = 0) out vec4 FragColor;

void main()
{
	vec2 TiledTexCoord = TexCoord * 40.0;
    float BaseWeight = 1.0 - TextureWeights[0] + TextureWeights[1] + TextureWeights[2];
    vec4 BaseTexture = texture(TerrainTextures[0], TexCoord) * BaseWeight;
    vec4 Texture1 = texture(TerrainTextures[TextureIDs.x], TexCoord) * TextureWeights[TextureIDs.x];
    vec4 Texture2 = texture(TerrainTextures[TextureIDs.y], TexCoord) * TextureWeights[TextureIDs.y];
    vec4 Texture3 = texture(TerrainTextures[TextureIDs.z], TexCoord) * TextureWeights[TextureIDs.z];
    vec4 FinalTexture = BaseTexture + Texture1 + Texture2 + Texture3;
    float CosineBetweenLight = dot(Normal, LightInfo.LightDirection);

    // TODO: Support Normal Map and Specular Map
    FragColor = (FinalTexture - FinalTexture + texture(TerrainTextures[0], TiledTexCoord)) * CosineBetweenLight;
}