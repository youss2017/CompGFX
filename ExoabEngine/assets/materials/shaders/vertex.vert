#version 450

#extension GL_ARB_shader_draw_parameters : require

struct Vertex
{
	float x, y, z;
	int nx_ny_nz_padding;
	// [16bits=tu][16bits=tv]
	uint tu_tv;
};

struct ObjectData
{
	mat4 m_Model;
	mat4 m_NormalModel;
	mat4 m_View;
	mat4 m_Projection;
};

struct VkDrawIndexedIndirectCommand {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

struct DrawData
{
	VkDrawIndexedIndirectCommand command;
	uint ObjectDataIndex;
};

layout (std430, set = 0, binding = 0) readonly buffer VerticesSSBO
{
	Vertex u_Vertices[];
};

layout (std430, set = 0, binding = 1) readonly buffer ObjectDataSSBO
{
	ObjectData u_ModelData[];
};

layout (std430, set = 0, binding = 2) readonly buffer DrawSSBO
{
	DrawData u_Draws[];
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;

// Since shaders don't pointers (therefore no references &), we must copy the Vertex which isn't allowed with int8 and float16
// therefore the following is the next best thing
#define vertex u_Vertices[gl_VertexIndex]

#define decompress_normal(compression) (vec3(int(compression >> 24), int(compression >> 16) & 0xff, int(compression >> 8) & 0xff) * 127.0)
#define decompress_texcoord(compression) (vec2(float(compression >> 16), float(compression & 0xffff)) / 65535.0)

void main()
{
	DrawData draw = u_Draws[gl_DrawIDARB];
	ObjectData od = u_ModelData[draw.ObjectDataIndex];
	Normal = mat3(od.m_NormalModel) * decompress_normal(vertex.nx_ny_nz_padding);
	TexCoord = decompress_texcoord(vertex.tu_tv);
	gl_Position = od.m_Projection * od.m_View * od.m_Model * vec4(vertex.x, vertex.y, vertex.z, 1.0);
}
