#version 450 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 color;

layout(push_constant) uniform uPushConstant {
    vec2 invDisplaySize;
} pc;

layout(location = 0) out struct {
    vec4 color;
    vec2 texCoord;
} vout;

void main()
{
    vout.color = color;
    vout.texCoord = texCoord;

    vec2 pos = position.xy * pc.invDisplaySize + vec2(-1.0, 1.0);
    gl_Position = vec4(pos, 0, 1);
}
