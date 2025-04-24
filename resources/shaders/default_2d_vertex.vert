#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec2 tilingFactor;
layout (location = 3) in vec4 color;
layout (location = 4) in uint texIndex;

layout (std140, binding = 0) uniform PushConstants {
    mat4 mvp;
} pushConstants;

struct VertexOutput
{
    vec4 color;
    vec2 texCoord;
    vec2 tilingFactor;
};

layout (location = 0) out VertexOutput vout;
layout (location = 3) out flat uint vTexIndex;

void main()
{
    vout.color = color;
    vout.texCoord = texCoord;
    vout.tilingFactor = tilingFactor;
    vTexIndex = texIndex;
    gl_Position = pushConstants.mvp * vec4(position, 1.0);
}