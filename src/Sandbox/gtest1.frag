#version 450

layout (location = 0) in vec2 TexCoordinates;
layout (location = 0) out vec4 FragColor;
layout (binding = 0) uniform sampler2D DisplayImage;

void main() {
    FragColor = texture(DisplayImage, TexCoordinates);
}
