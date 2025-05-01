#version 450 core
layout(location = 0) out vec4 oColor;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(location = 0) in struct {
    vec4 color;
    vec2 texCoord;
} vin;

void main()
{
    oColor = vin.color * texture(sTexture, vin.texCoord.st);
}
