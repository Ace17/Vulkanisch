#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set=0, binding=1) uniform sampler2D inputPicture;
layout(set=0, binding=2) uniform sampler2D bloomPicture;

void main()
{
  vec3 result = texture(inputPicture, uv).rgb + texture(bloomPicture, uv).rgb;

  // tone mapping
  const float gamma = 2.2;
  vec3 hdrColor = result;

  // reinhard tone mapping
  vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

  // gamma correction
  mapped = pow(mapped, vec3(1.0 / gamma));

  outColor = vec4(mapped, 1.0);
}

