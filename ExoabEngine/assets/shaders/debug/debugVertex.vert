#version 450 core
#extension GL_EXT_scalar_block_layout : enable

layout (location = 0) out vec3 color;

layout (scalar, push_constant) uniform pushblock {
    vec3 inOffset;
	vec3 inScale;
	vec3 inColor;
	mat4 u_ProjView;
};

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
    color = inColor;
    gl_Position = u_ProjView * vec4((CubeVertices[gl_VertexIndex] * inScale) + inOffset, 1.0);
}