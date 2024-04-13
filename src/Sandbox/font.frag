#version 450

layout (location = 0) in vec2 inUv;

 layout (binding = 0) uniform sampler2D texSampler;

layout (location = 0) out vec3 FragColor;

void main() {
    FragColor = texture(texSampler, inUv).rgb;
}