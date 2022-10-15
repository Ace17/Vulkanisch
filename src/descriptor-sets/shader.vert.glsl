#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding=0, std140) uniform MyDescriptorSet
{
  vec4 color;
  vec2 tx;
} UniformBlock;

void main()
{
    gl_Position = vec4(inPosition + UniformBlock.tx, 0.0, 1.0);
    fragColor = inColor;
}

// vim: syntax=glsl
