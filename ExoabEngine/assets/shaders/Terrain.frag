#version 450 core

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 Normal;
layout (location = 1) in flat ivec3 TextureIDs;
layout (location = 2) in vec3 TextureWeights;
layout (location = 3) in vec2 TexCoord;

// The first texture is the base texture
layout (binding = 1) uniform sampler2D TerrainTextures[];

layout (scalar, binding = 2) uniform light {
    vec3 u_LightDirection;
};

layout (location = 0) out vec4 FragColor;

void main()
{
	vec2 TiledTexCoord = TexCoord * 40.0;
    float BaseWeight = 1.0 - TextureWeights[0] + TextureWeights[1] + TextureWeights[2];
    vec4 BaseTexture = texture(TerrainTextures[nonuniformEXT(0)], TexCoord) * BaseWeight;
    vec4 Texture1 = (TextureIDs.x == -1) ? vec4(0.0) : (texture(TerrainTextures[nonuniformEXT(TextureIDs.x)], TexCoord) * TextureWeights[TextureIDs.x]);
    vec4 Texture2 = (TextureIDs.y == -1) ? vec4(0.0) : (texture(TerrainTextures[nonuniformEXT(TextureIDs.y)], TexCoord) * TextureWeights[TextureIDs.y]);
    vec4 Texture3 = (TextureIDs.z == -1) ? vec4(0.0) : (texture(TerrainTextures[nonuniformEXT(TextureIDs.z)], TexCoord) * TextureWeights[TextureIDs.z]);
    vec4 FinalTexture = BaseTexture + Texture1 + Texture2 + Texture3;
    float diffuse = -dot(Normal, u_LightDirection);

    // TODO: Support Normal Map and Specular Map
    FragColor = FinalTexture * diffuse;
}