#version 450 core

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable

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

layout (set = 1, binding = 0) uniform sampler2D textures[];
layout (set = 1, binding = 1) uniform sampler2D shadowMap;

const layout (constant_id = 0) int MAX_LIGHTS = 1;

struct Light {
	vec3 u_Position;
	vec3 u_Color;
	float u_Range;
	float u_AmbientStrength;
};

layout (scalar, set = 1, binding = 2) uniform LightData {
	vec3 u_CameraPosition;
	Light u_Lights[MAX_LIGHTS];
};

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in flat uint TextureID;
layout (location = 3) in vec3 FragPos;
layout (location = 4) in float SpecularStrength;

layout (location = 0) out vec4 FragColor;

void main() {
	
	for(int i = 0; i < MAX_LIGHTS; i++) {
		vec4 objectColor = texture(textures[TextureID], TexCoord);
		vec3 ambient = u_Lights[i].u_Color * u_Lights[i].u_AmbientStrength;
		
		vec3 lightDirection = normalize(u_Lights[i].u_Position - FragPos);
		float diffuseFactor = max(dot(Normal, lightDirection), 0.0);
		vec3 diffuse = u_Lights[i].u_Color * diffuseFactor;

		// from frag position to camera position
		vec3 viewDirection = normalize(u_CameraPosition - FragPos);
		vec3 reflectDir = reflect(-lightDirection, Normal);

		float spec = pow(max(dot(viewDirection, reflectDir), 0.0), 64);
		vec3 specular = max(SpecularStrength * spec * u_Lights[i].u_Color, vec3(0.0));

		FragColor = vec4((ambient + diffuse + specular), 1.0) * objectColor;
	}

}	