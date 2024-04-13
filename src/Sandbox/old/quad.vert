#version 450

layout(location = 0) out vec2 UV;
layout(location = 1) out vec4 Color;

// const vec4 vertices[] = {
//     vec4(-1, -1, 0, 0),
//     vec4(-1, +1, 0, 1),
//     vec4(+1, +1, 1, 1),
//     vec4(-1, -1, 0, 0),
//     vec4(+1, +1, 1, 1),
//     vec4(+1, -1, 1, 0)
// };

struct Quad {
    vec4 position;
    // w==1 ? use color : use texture
    vec4 color; 
};

layout(binding = 1) readonly buffer QuadData {
    /*
        Elements [0-1] are start coordinates
        Elements [2-3] are end coordinates
        QuadInfo uses pixel coordinates
    */
    Quad quads[];
};

layout (push_constant) uniform pushblock {
    vec2 OutputResolution;
};

void main() {
    Quad quad = quads[gl_InstanceIndex];
    Color = quad.color;

    // convert from [0, resolution] to [-1, 1]
    quad.position = uvec4(quad.position) % (uvec4(OutputResolution.xyxy) + 1);
    vec2 start = quad.position.xy / (OutputResolution * 0.5) - 1.0;
    vec2 end = (quad.position.xy + quad.position.zw) / (OutputResolution * 0.5) - 1.0;

    vec2 pos;
    switch(gl_VertexIndex) {
        case 0: 
            pos = vec2(start);
            UV = vec2(0, 1);
            break;
        case 1: 
            pos = vec2(start.x, end.y); 
            UV = vec2(0, 0);
            break;
        case 2: 
            pos = vec2(end); 
            UV = vec2(1, 0);
            break;
        case 3: 
            pos = vec2(start); 
            UV = vec2(0, 1);
            break;
        case 4: 
            pos = vec2(end); 
            UV = vec2(1, 0);
            break;
        case 5: 
            pos = vec2(end.x, start.y); 
            UV = vec2(1, 1);
            break;
    }

    gl_Position = vec4(pos, 0.0, 1.0);
}