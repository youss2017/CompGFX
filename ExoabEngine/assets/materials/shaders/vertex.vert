#version 450

#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex
{
	float16_t x, y, z;
	uint8_t nx, ny, nz, nw;
	float16_t tu, tv;
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
	uint16_t ObjectDataIndex;
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
#define draw u_Draws[gl_DrawIDARB]

vec4 InvertQuat(vec4 quat)
{
	return vec4(quat.x, quat.yzw * -1.0);
}

vec4 MulQuat(vec4 q1, vec4 q2)
{
    float x =  q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
    float y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
    float z =  q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
    float w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;
	return vec4(x, y, z, w);
}

void main()
{
	ObjectData od = u_ModelData[int(draw.ObjectDataIndex)];
	Normal =  mat3(od.m_NormalModel) * (vec3(int(vertex.nx), int(vertex.ny), int(vertex.nz)) / 127.0 - 1.0);
	TexCoord = vec2(float(vertex.tu), float(vertex.tv));
	gl_Position = u_Scene.m_Projection * u_Scene.m_View * od.m_Model * vec4(vertex.x, vertex.y, vertex.z, 1.0);
}
