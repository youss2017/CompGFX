#version 450

layout (location = 0) out vec2 TexCoordinates;

const vec4 positions_and_uvs[] = {
    {-1.0, -1.0, 0.0, 0.0},
    {-1.0, +1.0, 0.0, 1.0},
    {+1.0, +1.0, 1.0, 1.0},
    {-1.0, -1.0, 0.0, 0.0},
    {+1.0, +1.0, 1.0, 1.0},
    {+1.0, -1.0, 1.0, 0.0}
};

void main() {
    TexCoordinates = positions_and_uvs[gl_VertexIndex].zw;
    gl_Position = vec4(positions_and_uvs[gl_VertexIndex].xy, 0.0, 1.0);
}
