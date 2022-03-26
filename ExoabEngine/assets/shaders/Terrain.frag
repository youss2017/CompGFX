#version 450 core

#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : disable

layout (location = 0) in vec3 Normal;
layout (location = 1) in flat ivec3 TextureIDs;
layout (location = 2) in vec3 TextureWeights;
layout (location = 3) in vec2 TexCoord;
layout (location = 4) in vec4 LightSpacePos;

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

// The first texture is the base texture
layout (binding = 1) uniform sampler2D TerrainTextures[];
layout (binding = 2) uniform sampler2D shadowMap;

// works for both positive and negative numbers and no problem when a == b,  purple
float smin(float a, float b, float k)
{
    float x = exp(-k * a);
    float y = exp(-k * b);
    return (a * x + b * y) / (x + y);
}

float ShadowCalculation()
{
    vec3 pos = (LightSpacePos.xyz / LightSpacePos.w) * 0.5 + 0.5;
    if(pos.z > 1.0) {
        pos.z = 1.0;
    }
    vec2 shadowSize = textureSize(shadowMap, 0);
    vec2 shadowTexel = 1.0 / shadowSize;
    
    const int pcfCount = 2;
    const float totalTexels = (pcfCount * 2.0 + 1.0) * (pcfCount * 2.0 + 1.0);
    float total = 0.0;

    for(int x = -pcfCount; x <= pcfCount; x++) {
        for(int y = -pcfCount; y <= pcfCount; y++) {
            float depth = texture(shadowMap, pos.xy + (vec2(x, y) * shadowTexel * 0.88)).r;
            if(pos.z > depth) {
                total += 1.0;
            }
        }       
    }

    return total / totalTexels;
}

layout (location = 0) out vec4 FragColor;

void main()
{
	vec2 TiledTexCoord = TexCoord * 40.0;
    float BaseWeight = 1.0 - TextureWeights[0] + TextureWeights[1] + TextureWeights[2];
    vec4 BaseTexture = texture(TerrainTextures[0], TexCoord) * BaseWeight;
    vec4 Texture1 = (TextureIDs.x == -1) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.x], TexCoord) * TextureWeights[TextureIDs.x]);
    vec4 Texture2 = (TextureIDs.y == -1) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.y], TexCoord) * TextureWeights[TextureIDs.y]);
    vec4 Texture3 = (TextureIDs.z == -1) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.z], TexCoord) * TextureWeights[TextureIDs.z]);
    vec4 FinalTexture = BaseTexture + Texture1 + Texture2 + Texture3;
    float diffuse = dot(Normal, u_LightDirection.xyz);

    // TODO: Support Normal Map and Specular Map
    FragColor = FinalTexture * diffuse * clamp((1.0 - ShadowCalculation()), 0.4, 1.0);
}