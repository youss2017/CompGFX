#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex
{
	vec4 position;
	vec4 normal;
	vec2 texcoord;
	float padding[2];
};

struct SceneData
{
	mat4 m_View;
	mat4 m_Projection;
};

struct ObjectData
{
	vec3 bounding_sphere_center;
	float bounding_sphere_radius;
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
	uint ObjectDataIndex;
	uint TexIndex;
};