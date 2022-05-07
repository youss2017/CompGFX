#version 450

layout (push_constant) uniform ProjView {
    mat4 u_ProjView;
};

layout (location = 0) out vec3 textureCoords;

// This must be equal to far plane
const float SIZE = 500;
	
const vec3 positions[36] = {        
	vec3(-SIZE,  SIZE, -SIZE),
	vec3(-SIZE, -SIZE, -SIZE),
	vec3(+SIZE, -SIZE, -SIZE),
	vec3(+SIZE, -SIZE, -SIZE),
	vec3(+SIZE,  SIZE, -SIZE),
	vec3(-SIZE,  SIZE, -SIZE),
	vec3(-SIZE, -SIZE,  SIZE),
	vec3(-SIZE, -SIZE, -SIZE),
	vec3(-SIZE,  SIZE, -SIZE),
	vec3(-SIZE,  SIZE, -SIZE),
	vec3(-SIZE,  SIZE,  SIZE),
	vec3(-SIZE, -SIZE,  SIZE),
	vec3(+SIZE, -SIZE, -SIZE),
	vec3(+SIZE, -SIZE,  SIZE),
	vec3(+SIZE,  SIZE,  SIZE),
	vec3(+SIZE,  SIZE,  SIZE),
	vec3(+SIZE,  SIZE, -SIZE),
	vec3(+SIZE, -SIZE, -SIZE),
	vec3(-SIZE, -SIZE,  SIZE),
	vec3(-SIZE,  SIZE,  SIZE),
	vec3(+SIZE,  SIZE,  SIZE),
	vec3(+SIZE,  SIZE,  SIZE),
	vec3(+SIZE, -SIZE,  SIZE),
	vec3(-SIZE, -SIZE,  SIZE),
	vec3(-SIZE,  SIZE, -SIZE),
	vec3(+SIZE,  SIZE, -SIZE),
	vec3(+SIZE,  SIZE,  SIZE),
	vec3(+SIZE,  SIZE,  SIZE),
	vec3(-SIZE,  SIZE,  SIZE),
	vec3(-SIZE,  SIZE, -SIZE),
	vec3(-SIZE, -SIZE, -SIZE),
	vec3(-SIZE, -SIZE,  SIZE),
	vec3(+SIZE, -SIZE, -SIZE),
	vec3(+SIZE, -SIZE, -SIZE),
	vec3(-SIZE, -SIZE,  SIZE),
	vec3(+SIZE, -SIZE,  SIZE)
};

void main() {
    gl_Position = u_ProjView * vec4(positions[gl_VertexIndex], 1.0);
    textureCoords = (positions[gl_VertexIndex]) * vec3(1, -1, 1);
}