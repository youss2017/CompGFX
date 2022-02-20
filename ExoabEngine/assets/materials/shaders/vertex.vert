#version 450

#extension GL_ARB_shader_draw_parameters : require

struct Vertex
{
	float x, y, z;
	float nx, ny, nz;
	float tu, tv;
};

struct SceneData
{
	mat4 m_View;
	mat4 m_Projection;
};

struct ObjectData
{
	mat4 m_Model;
	mat4 m_NormalModel;
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
	uint SceneDataIndex;
	uint ObjectDataIndex;
};

layout (std430, set = 0, binding = 0) readonly buffer VerticesSSBO
{
	Vertex u_Vertices[];
};

layout (set = 0, binding = 1) uniform SceneDataUBO
{
	SceneData u_Scene;
};

layout (set = 0, binding = 2) readonly buffer ObjectDataSSBO
{
	ObjectData u_ModelData[];
};

layout (std430, set = 0, binding = 3) readonly buffer DrawSSBO
{
	DrawData u_Draws[];
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;

// Since shaders don't pointers (therefore no references &), we must copy the Vertex which isn't allowed with int8 and float16
// therefore the following is the next best thing
#define vertex u_Vertices[gl_VertexIndex]
#
void main()
{
	DrawData draw = u_Draws[gl_DrawIDARB];
	ObjectData od = u_ModelData[draw.ObjectDataIndex];
	Normal = mat3(od.m_NormalModel) * vec3(vertex.nx, vertex.ny, vertex.nz);
	TexCoord = vec2(vertex.tu, vertex.tv);
	gl_Position = u_Scene.m_Projection * u_Scene.m_View * od.m_Model * vec4(vertex.x, vertex.y, vertex.z, 1.0);
}
