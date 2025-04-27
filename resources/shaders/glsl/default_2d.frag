#version 450 core
layout (location = 0) out vec4 oColor;

struct VertexOutput
{
    vec4 color;
    vec2 texCoord;
    vec2 tilingFactor;
};

layout (location = 0) in VertexOutput vin;
layout (location = 3) in flat uint vTexIndex;

layout (binding = 0) uniform sampler2D uTextures[16];

void main()
{
    vec4 texColor = vin.color;
    texColor *= texture(uTextures[int(vTexIndex)], vin.texCoord * vin.tilingFactor);
    oColor = texColor;
}