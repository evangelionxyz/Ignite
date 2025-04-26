#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

struct Vertex
{
    vec3 position;
    vec3 color;
};

layout(location = 0) out Vertex vout;

void main()
{
    gl_Position = vec4(position, 1.0);
    vout.position = position;
    vout.color = color;
}