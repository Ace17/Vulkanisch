#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D inputPicture0;
layout(set=0, binding=1) uniform sampler2D inputPicture1;

void main()
{
  vec3 result = texture(inputPicture0, uv).rgb + texture(inputPicture1, uv).rgb;

  // tone mapping
  const float gamma = 1.0;
  vec3 hdrColor = result;

  // reinhard tone mapping
  vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

  // gamma correction
  mapped = pow(mapped, vec3(1.0 / gamma));

  outColor = vec4(mapped, 1.0);
}

