#version 450
layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 Color;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D blitImage; 

void main() {
    if(Color[3] == 1) {
        FragColor = Color;
    } else {
        FragColor = texture(blitImage, inUV);
    }
}