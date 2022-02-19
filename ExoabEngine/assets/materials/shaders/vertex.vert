#version 450

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_ARB_shader_draw_parameters : require

struct Vertex
{
	float x, y, z;
	int8_t nx, ny, nz;
	float16_t tu, tv;	
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

#define decompress_normal(x, y, z) (vec3(int(x), int(y), int(z)) * 127.0)

void main()
{
	DrawData draw = u_Draws[gl_DrawIDARB];
	ObjectData od = u_ModelData[draw.ObjectDataIndex];
	Normal = mat3(od.m_NormalModel) * decompress_normal(vertex.nx, vertex.ny, vertex.nz);
	TexCoord = vec2(vertex.tu, vertex.tv);
	gl_Position = od.m_Projection * od.m_View * od.m_Model * vec4(vertex.x, vertex.y, vertex.z, 1.0);
}
