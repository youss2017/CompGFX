#version 450
#include "types.h"
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#define sizeof(Type) (uint64_t(Type(uint64_t(0))+1))

layout (std430, set = 0, binding = 0) readonly buffer VerticesSSBO
{
	Vertex u_Vertices[];
};

layout (std430, set = 0, binding = 1) uniform SceneDataUBO
{
	SceneData u_Scene;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer ObjectData_BlockType {
	ObjectData v[]; 
};

/*
layout (std430, set = 0, binding = 2) readonly buffer ObjectDataSSBO
{
	ObjectData u_ModelData[];
};
*/
layout(push_constant) uniform pushblock {
	uint64_t u_ModelDataPointer;
};

layout (std430, set = 0, binding = 3) readonly buffer DrawSSBO
{
	DrawData u_Draws[];
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;
layout (location = 2) out flat uint TexIndex;

// Since shaders don't pointers (therefore no references &), we must copy the Vertex which isn't allowed with int8 and float16
// therefore the following is the next best thing
#define vertex u_Vertices[gl_VertexIndex]
#define draw u_Draws[gl_DrawIDARB]

vec4 InvertQuat(vec4 quat)
{
	return vec4(quat.x, quat.yzw * -1.0);
}

vec4 MulQuat(vec4 q1, vec4 q2)
{
	float a = q1.x; float e = q2.x;
	float b = q1.y; float f = q2.y;
	float c = q1.z; float g = q2.z;
	float d = q1.w; float h = q2.w;

	float r = (a*e) - (b*f) - (c*g) - (d*h);
	float i = (a*f) + (b*e) + (c*h) - (d*g);
	float j = (a*g) - (b*h) + (c*e) + (d*f);
	float k = (a*h) + (b*g) - (c*f) + (d*e);
	return vec4(r, i, j, k);
}

void main()
{
	ObjectData_BlockType objData = ObjectData_BlockType(u_ModelDataPointer);
	ObjectData od = objData.v[int(draw.ObjectDataIndex)];
	Normal =  mat3(od.m_NormalModel) * vertex.normal.xyz;
	TexCoord = vertex.texcoord;
	TexIndex = uint(draw.TexIndex);
	gl_Position = u_Scene.m_Projection * u_Scene.m_View * od.m_Model * vertex.position;
}
