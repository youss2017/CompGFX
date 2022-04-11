#version 450 core

const vec4 quad[] = 
{
    // triangle 1\n"
    vec4(-1, 1, 0, 1),
    vec4(1, -1, 1, 0),
    vec4(-1, -1, 0, 0),
    // triangle 2\n"
    vec4(-1, 1, 0, 1),
    vec4(1, 1, 1, 1),
    vec4(1, -1, 1, 0)
};

layout (location = 0) out vec2 TexCoord;

void main()
{
    vec4 Vertex = quad[gl_VertexIndex];
    gl_Position = vec4(Vertex.xy, 0.0, 1.0);
    TexCoord = Vertex.zw;
}