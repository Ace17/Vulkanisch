#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;

layout(set=0, binding=0, std140) uniform MyDescriptorSet
{
  mat4x4 model;
  mat4x4 view;
  mat4x4 proj;
} UniformBlock;


void main()
{
  mat4x4 tx = UniformBlock.proj * UniformBlock.view * UniformBlock.model;
  gl_Position = tx * vec4(inPosition, 1);
  outUv = vec2(0);
  outNormal = (UniformBlock.model * vec4(inNormal, 0)).xyz;
}

