#version 450 core
#include "../pointer.h"

layout (location = 0) out vec3 color;

layout (push_constant) uniform pushblock {
	mat4 u_ProjView;
	uint64_t u_DebugObjectPtr;
	uint u_DebugInstanceOffset; 
};

struct DebugObject {
	vec3 inOffset;
	vec3 inScale;
	vec3 inColor;
};

DECLARE_POINTER_ARRAY(DebugObject, scalar);

const vec3 CubeVertices [36] = {
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3(+1.0, -1.0, -1.0),
	vec3(+1.0, -1.0, -1.0),
	vec3(+1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3(+1.0, -1.0, -1.0),
	vec3(+1.0, -1.0,  1.0),
	vec3(+1.0,  1.0,  1.0),
	vec3(+1.0,  1.0,  1.0),
	vec3(+1.0,  1.0, -1.0),
	vec3(+1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(+1.0,  1.0,  1.0),
	vec3(+1.0,  1.0,  1.0),
	vec3(+1.0, -1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(+1.0,  1.0, -1.0),
	vec3(+1.0,  1.0,  1.0),
	vec3(+1.0,  1.0,  1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3(+1.0, -1.0, -1.0),
	vec3(+1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3(+1.0, -1.0,  1.0)
};

void main() {
	DebugObject obj = DebugObject_T(u_DebugObjectPtr).v[gl_InstanceIndex + u_DebugInstanceOffset];
    color = obj.inColor;
    gl_Position = u_ProjView * vec4((CubeVertices[gl_VertexIndex] * obj.inScale) + obj.inOffset, 1.0);
}