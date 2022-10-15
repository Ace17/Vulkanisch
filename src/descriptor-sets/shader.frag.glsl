#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding=0, std140) uniform MyDescriptorSet
{
  vec4 color;
  vec2 tx;
} UniformBlock;

void main()
{
    outColor = vec4(fragColor, 1.0) + UniformBlock.color;
}

// vim: syntax=glsl
