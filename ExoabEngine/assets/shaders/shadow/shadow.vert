#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference_uvec2 : require
#include "../pointer.h"
#include "../types.h"

layout (scalar, binding = 0) restrict readonly buffer VerticesSSBO
{
	Vertex u_Vertices[];
};

layout (scalar, binding = 1) readonly buffer ObjectDataSSBO
{
	GeometryData u_ModelData[];
};

layout (scalar, binding = 2) readonly buffer DrawSSBO
{
	DrawData u_Draws[];
};

layout (binding = 3) uniform MVBuffer {
	mat4 u_ShadowTOffset;
	mat4 u_ProjView;
};

void main() {
	// TODO: Culling with orthographic projection
	mat4 model = InstanceData_T(u_ModelData[u_Draws[gl_DrawIDARB].mGeometryDataIndex].mInstancePtr).v[gl_InstanceIndex].mModel;
	gl_Position = u_ProjView * model * vec4(u_Vertices[gl_VertexIndex].position, 1.0);
}