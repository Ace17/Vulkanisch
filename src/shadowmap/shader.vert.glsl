#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 fragPositionLightSpace;

layout(binding=1, std140) uniform MyDescriptorSet
{
  mat4x4 model;
  mat4x4 view;
  mat4x4 proj;
  mat4x4 lightMVP;
} UniformBlock;


void main()
{
  mat4x4 tx = UniformBlock.proj * UniformBlock.view * UniformBlock.model;
  gl_Position = tx * vec4(inPosition, 1);
  fragPositionLightSpace = UniformBlock.lightMVP * vec4(inPosition, 1);
  outUv = inUv;
}

