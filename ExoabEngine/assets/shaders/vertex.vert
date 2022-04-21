#version 450
#include "types.h"
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference_uvec2 : require

#extension GL_EXT_shader_explicit_arithmetic_types_int8 : disable
#extension GL_EXT_debug_printf : disable

layout (scalar, binding = 0) restrict readonly buffer VerticesSSBO
{
	Vertex u_Vertices[];
};

layout (scalar, binding = 1) uniform GlobalDataUBO
{
	float u_DeltaTime;
	float u_TimeFromStart;
	vec4 u_CameraForward;
	mat4 u_View;
	mat4 u_Projection;
	mat4 u_ProjView;
	mat4 u_LightSpace;
};

layout (scalar, binding = 2) readonly buffer ObjectDataSSBO
{
	GeometryData u_ModelData[];
};

layout (scalar, binding = 3) readonly buffer DrawSSBO
{
	DrawData u_Draws[];
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;
layout (location = 2) out flat uint TextureID;
layout (location = 3) out vec3 FragPos;
layout (location = 4) out float SpecularStrength;

// Since shaders don't pointers (therefore no references &), we must copy the Vertex which isn't allowed with int8 and float16
// therefore the following is the next best thing
#define vertex u_Vertices[gl_VertexIndex]
#define draw u_Draws[gl_DrawIDARB]

void main()
{
	GeometryData geoData = u_ModelData[draw.mGeometryDataIndex];
	InstanceData instance = InstanceData_T(geoData.mCulledInstancePtr).v[gl_InstanceIndex];
	Normal = normalize(instance.mNormalModel * vertex.normal);
	TexCoord = vertex.texcoord;
	TextureID = instance.mTextureIndex;
	FragPos = vec3(instance.mModel * vec4(vertex.position, 1.0));
	SpecularStrength = instance.mSpecularStrength;
	gl_Position = u_ProjView * vec4(FragPos, 1.0);
}
