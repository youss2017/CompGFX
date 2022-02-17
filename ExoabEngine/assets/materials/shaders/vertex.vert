#version 450

struct Vertex
{
	vec3 inPosition;
	vec3 inNormal;
	vec2 inTexCoord;	
};

layout (set = 0, binding = 0) readonly buffer VerticesSSBO
{
	Vertex vertices[];
};

//layout (push_constant) uniform RenderingState
//{
//	uint vertices_offset;
//};

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;

void main()
{
	Vertex v = vertices[gl_VertexIndex];
	Normal = v.inNormal;
	TexCoord = v.inTexCoord;
	gl_Position = vec4(v.inPosition, 1.0);//vec4(v.x, v.y, v.z, 1.0);
}
