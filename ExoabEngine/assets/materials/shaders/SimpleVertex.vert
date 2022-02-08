#version 450

//layout (location = 0) in vec2 Position;

struct Vertex {
	vec2 position;
	vec3 color;
};

Vertex vertices[] = {
	{vec2(-0.5, 0.5), vec3(1.0, 0.0, 0.0)},
	{vec2(0.0, -0.5), vec3(0.0, 1.0, 0.0)},
	{vec2(0.5, 0.5), vec3(0.0, 0.0, 1.0)}
};

layout (location = 0) out vec3 OutColor;

void main() {
	gl_Position = vec4(vertices[gl_VertexIndex].position * 2.0, 0.0, 1.0); //vec4(Position, 0.0, 1.0);
	OutColor = vertices[gl_VertexIndex].color;
}