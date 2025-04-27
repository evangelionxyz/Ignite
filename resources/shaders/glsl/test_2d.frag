#version 450 core
layout(location = 0) out vec4 oColor;

struct Vertex
{
    vec3 position;
    vec3 color;
};

layout(location = 0) in Vertex vin;

void main()
{
    oColor = vec4(vin.color, 1.0);
}