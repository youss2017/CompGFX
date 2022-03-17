#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
};

struct InstanceData {
	uint TexIndex[4];
    mat4 mModel;
};

#define INSTANCE_SIZE 80

layout(buffer_reference, scalar) buffer InstanceData_T {
    InstanceData v[];
};

struct GeometryData
{
	vec3 bounding_sphere_center;
	float bounding_sphere_radius;
    uint64_t mInstancePtr;
    uint64_t mCulledInstancePtr;
};

struct DrawData
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
    uint mGeometryDataIndex;
};