#version 440 core

#pragma shader_stage(vertex)

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 UV;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;

layout(binding = 0) uniform Cam_t
{
    mat4 World;
    mat4 Proj;
    mat4 View;
} Camera;

void main()
{
    gl_Position = Camera.View*Camera.Proj*Camera.World*vec4(Position.xyz, 1.f);
    outNormal = Normal.xyz;
    outUV = UV;
}
