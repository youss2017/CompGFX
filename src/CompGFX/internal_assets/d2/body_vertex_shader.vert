#version 450

layout (location = 0) out vec3 out_color_or_uv;
layout (location = 1) out float out_opacity;

struct DrawCall {
	vec4 position;
	vec3 color_or_uv;
	float opacity;
};

layout (binding = 0) readonly buffer vertex_buffer_s {
	DrawCall drawCalls[];
};

layout (binding = 1) readonly buffer transform_s {
	mat4 transform[];
} transforms;

vec3 quad_verts[] = {
	vec3(-0.5, 0.5, 0.0),
	vec3(0.5, 0.5, 0.0),
	vec3(-0.5, -0.5, 0.0),
	vec3(0.5, 0.5, 0.0),
	vec3(0.5, -0.5, 0.0),
	vec3(-0.5, -0.5, 0.0),
};

void main() {
	DrawCall draw = drawCalls[gl_InstanceIndex];
	vec3 vertex = quad_verts[gl_VertexIndex] + draw.position.xyz;
	gl_Position = transforms.transform[gl_InstanceIndex] * vec4(vertex, 1.0);
	out_color_or_uv = draw.color_or_uv;
	out_opacity = draw.opacity;
}