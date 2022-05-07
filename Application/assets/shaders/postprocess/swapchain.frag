#version 450 core
#include "tonemapping.glsl"

layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;
layout(set = 0, binding = 0) uniform sampler2D colorImage;

layout (push_constant) uniform pushblock {
    ivec2 u_ShowColor;
    vec2 zNear;
    vec2 zFar;
    vec2 u_Exposure;
};

float LinearizeDepth(in vec2 uv)
{
    float depth = texture(colorImage, uv).x;
    return (2.0 * zNear[0]) / (zFar[0] + zNear[0] - depth * (zFar[0] - zNear[0]));
}

void main()
{
    const float gamma = 2.2;
    if(u_ShowColor[0] == 1) {
        vec3 Color = texture(colorImage, TexCoord).rgb;
        Color *= u_Exposure[0];
        Color = reinhard(Color.rgb);
        Color = pow(Color.rgb, vec3(1.0 / gamma));
        FragColor = vec4(Color, 1.0);
    } else {
        //float c = LinearizeDepth(TexCoord);
        float depth = texture(colorImage, TexCoord).x;
        FragColor = vec4(depth, depth, depth, 1.0);
    }
}