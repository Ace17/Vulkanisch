#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 outUv;

layout(binding=1, std140) uniform MyDescriptorSet
{
    float angle;
} UniformBlock;


void main()
{
    const float s = sin(UniformBlock.angle);
    const float c = cos(UniformBlock.angle);
    float x = c * inPosition.x - s * inPosition.y;
    float y = s * inPosition.x + c * inPosition.y;
    gl_Position = vec4(x, y, 0.0, 1.0);
    outUv = inUv;
}

// vim: syntax=glsl
