#version 450 core
#include "../pointer.h"
#extension GL_EXT_debug_printf : disable

#define CUBE 0
#define QUAD 1

layout (location = 0) out vec3 color;

layout (push_constant) uniform pushblock {
	mat4 u_ProjView;
	uint64_t u_DebugObjectPtr;
	uint u_DebugInstanceOffset; 
};

struct DebugObject {
	vec3 inOffset;
	vec3 inScale;
	vec3 inPointAB[2];
	vec3 inColor;
	uint inObjIndex;
};

DECLARE_POINTER_ARRAY(DebugObject, scalar);

const vec3 CubeVertices [36] = {
	// Cube Vertices
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
	vec3(+1.0, -1.0,  1.0),
};

const vec2 QuadVertices[6] = {
	// Quad Vertices
	// Tri 1
	vec2(-1.0, -1.0),
	vec2(-1.0, 1.0), 
	vec2(1.0, -1.0), 
	// Tri 2
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0)
};

const uint QuadPointIndices[6] = { 0, 1, 1, 1, 0, 0 };
const vec3 QuadOffests[6] = {
	vec3(-0.5, 0.0, 0.0),
	vec3(-0.5, 0.0, 0.0),
	vec3(+0.5, 0.0, 0.0),
	vec3(+0.5, 0.0, 0.0),
	vec3(+0.5, 0.0, 0.0),
	vec3(-0.5, 0.0, 0.0),
};

void main() {
	DebugObject obj = DebugObject_T(u_DebugObjectPtr).v[gl_InstanceIndex + u_DebugInstanceOffset];
    color = obj.inColor;
	if(obj.inObjIndex == CUBE) {
    	gl_Position = u_ProjView * vec4((CubeVertices[gl_VertexIndex] * obj.inScale) + obj.inOffset, 1.0);
    } else if(obj.inObjIndex == QUAD) {
#if 1
    	// Somehow this is faster? memory reads really that bad.
    	switch(gl_VertexIndex) {
    		case 0:
		    	gl_Position = u_ProjView * vec4(obj.inPointAB[0] - vec3(0.5, 0.0, 0.0), 1.0);
    			break;
    		case 1:
		    	gl_Position = u_ProjView * vec4(obj.inPointAB[1] - vec3(0.5, 0.0, 0.0), 1.0);
    			break;
    		case 2:
		    	gl_Position = u_ProjView * vec4(obj.inPointAB[1] + vec3(0.5, 0.0, 0.0), 1.0);
    			break;
    		case 3:
		    	gl_Position = u_ProjView * vec4(obj.inPointAB[1] + vec3(0.5, 0.0, 0.0), 1.0);
    			break;
    		case 4:
		    	gl_Position = u_ProjView * vec4(obj.inPointAB[0] + vec3(0.5, 0.0, 0.0), 1.0);
    			break;
    		case 5:
		    	gl_Position = u_ProjView * vec4(obj.inPointAB[0] - vec3(0.5, 0.0, 0.0), 1.0);
    			break;
    		default:
    			break;
    	}
#else
		gl_Position = u_ProjView * vec4(obj.inPointAB[QuadPointIndices[gl_VertexIndex]] + QuadOffests[gl_VertexIndex], 1.0);
#endif
    }
}