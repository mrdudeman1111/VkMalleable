#version 440 core

#pragma shader_stage(fragment)

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec2 UV;

layout(location = 0) out vec4 outCol;

void main()
{
    outCol = vec4(1.f, 1.f, 1.f, 1.f);
}
