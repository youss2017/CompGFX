#version 450 core

#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : disable

layout (location = 0) in flat uvec3 TextureIDs;
layout (location = 1) in vec3 TextureWeights;
layout (location = 2) in vec2 TexCoord;
layout (location = 3) in vec4 LightSpacePos;
layout (location = 4) in mat3 TBN;

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
layout (binding = 3) uniform sampler2D shadowMap;
layout (binding = 4) uniform sampler2D normalMap;


layout (constant_id = 0) const int pcfCount = 1;

float ShadowCalculation()
{
    vec3 pos = (LightSpacePos.xyz / LightSpacePos.w) * 0.5 + 0.5;
    if(pos.z > 1.0) {
        pos.z = 1.0;
    }
    vec2 shadowSize = textureSize(shadowMap, 0);
    vec2 shadowTexel = 1.0 / shadowSize;
    
    const float totalTexels = (pcfCount * 2.0 + 1.0) * (pcfCount * 2.0 + 1.0);
    float total = 0.0;

    for(int x = -pcfCount; x <= pcfCount; x++) {
        for(int y = -pcfCount; y <= pcfCount; y++) {
            float depth = textureGather(shadowMap, pos.xy + (vec2(x, y) * shadowTexel) * 0.5).r;
            if(pos.z > depth) {
                total += 1.0;
            }
        }       
    }

    return 1.0 - total / totalTexels;
}

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

    // TODO: Support Specular Map
    FragColor = FinalTexture * diffuse * clamp(ShadowCalculation(), 0.5, 1.0) + ambient;
}