#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

// Instancing Data
layout (location = 3) in vec2 XYOffset;

layout (binding = 0) uniform View_Projection {
	mat4 u_View;
	mat4 u_Projection;
};

layout (binding = 1) uniform Model {
	mat4 u_Model;
	mat4 u_NormalModel; // transpose(inverse(u_Model))
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;

void main() {
	Normal = mat3(u_NormalModel) * inNormal;
	TexCoord = inTexCoord;
	vec4 InstancePosition = vec4(inPosition.xy + XYOffset, inPosition.z, 1.0);
	gl_Position = u_Projection * u_View * u_Model * InstancePosition;
	//gl_Position = u_Projection * u_View * u_Model * vec4(inPosition, 1.0);
}
