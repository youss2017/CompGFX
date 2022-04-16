#version 450 core

#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : disable

const layout (constant_id = 0) int pcfCount = 1;
const layout (constant_id = 1) int MAX_LIGHTS = 1;

layout (location = 0) in flat uvec3 TextureIDs;
layout (location = 1) in vec3 TextureWeights;
layout (location = 2) in vec2 TexCoord;
layout (location = 3) in vec4 LightSpacePos;
layout (location = 4) in vec3 FragPos;
layout (location = 5) in mat3 TBN;

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

struct Light {
	vec3 u_Position;
	vec3 u_Color;
	float u_Range;
	float u_AmbientStrength;
};

layout (scalar, binding = 5) uniform LightData {
	vec3 u_CameraPosition;
	Light u_Lights[MAX_LIGHTS];
};


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
    vec4 Texture1 = (TextureIDs.x == 0) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.x], TiledTexCoord) * TextureWeights[TextureIDs.x]);
    vec4 Texture2 = (TextureIDs.y == 0) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.y], TiledTexCoord) * TextureWeights[TextureIDs.y]);
    vec4 Texture3 = (TextureIDs.z == 0) ? vec4(0.0) : (texture(TerrainTextures[TextureIDs.z], TiledTexCoord) * TextureWeights[TextureIDs.z]);
    vec4 objectColor = BaseTexture + Texture1 + Texture2 + Texture3;
    const float SpecularStrength = 0.1;

    vec4 color = vec4(0.0);
    float shadowRatio = 0.0;
    for(int i = 0; i < MAX_LIGHTS; i++) {
		vec3 ambient = u_Lights[i].u_Color * u_Lights[i].u_AmbientStrength;
		shadowRatio += length(ambient);

		vec3 lightDirection = normalize(u_Lights[i].u_Position - FragPos);
		float diffuseFactor = max(dot(normal, lightDirection), 0.0);
		vec3 diffuse = u_Lights[i].u_Color * diffuseFactor;

		// from frag position to camera position
		vec3 viewDirection = normalize(u_CameraPosition - FragPos);
		vec3 reflectDir = reflect(-lightDirection, normal);

		float spec = pow(max(dot(viewDirection, reflectDir), 0.0), 16);
		vec3 specular = max(SpecularStrength * spec * u_Lights[i].u_Color, vec3(0.0));

		color += vec4((ambient + diffuse + specular), 1.0) * objectColor;
    }
    
    FragColor = color * clamp(ShadowCalculation(), shadowRatio, 1.0);
}