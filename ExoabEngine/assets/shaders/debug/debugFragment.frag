#version 450 core

layout (location = 0) in vec3 color;
layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(color, 1.0);
}