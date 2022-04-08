#version 450 core

#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : disable

layout (location = 0) in flat uvec3 TextureIDs;
layout (location = 1) in vec3 TextureWeights;
layout (location = 2) in vec2 TexCoord;
layout (location = 3) in mat3 TBN;

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

// The first texture is the base texture
layout (binding = 2) uniform sampler2D TerrainTextures[];
layout (binding = 3) uniform sampler2D normalMap;

layout (location = 0) out vec4 FragColor;

void main()
{
	vec2 TiledTexCoord = TexCoord * 40.0;
    vec3 normal = texture(normalMap, TiledTexCoord).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(TBN * normal);

    float BaseWeight = 1.0 - TextureWeights[0] + TextureWeights[1] + TextureWeights[2];
    vec4 BaseTexture = texture(TerrainTextures[0], TiledTexCoord) * BaseWeight;
    vec4 Texture1 = (TextureIDs.x == -1) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.x], TiledTexCoord) * TextureWeights[TextureIDs.x]);
    vec4 Texture2 = (TextureIDs.y == -1) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.y], TiledTexCoord) * TextureWeights[TextureIDs.y]);
    vec4 Texture3 = (TextureIDs.z == -1) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.z], TiledTexCoord) * TextureWeights[TextureIDs.z]);
    vec4 FinalTexture = BaseTexture + Texture1 + Texture2 + Texture3;
    float ambient = 0.01;
    float diffuse = dot(normal, u_LightDirection.xyz);

    FragColor = FinalTexture * diffuse + ambient;
}