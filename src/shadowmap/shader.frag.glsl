#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 fragPositionLightSpace;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D shadowMapSampler;

vec3 checkerTexture(vec2 uv)
{
  int col = int(floor(uv.x * 8));
  int row = int(floor(uv.y * 8));
  float c = col % 2 == row % 2 ? 1 : 0;
  return vec3(0, c, 1);
}

float computeShadow(vec4 pos)
{
  if (pos.z <= -1.0 || pos.z >= +1.0 || pos.w <= 0.0)
    return 1;

  vec2 uv = pos.xy * 0.5 + 0.5;
  uv.y = 1-uv.y;

  float dist = texture(shadowMapSampler, uv).r;

  if (dist >= pos.z - 0.01)
    return 1;

  return 0.5; // in shadow
}

void main()
{
  float shadow = computeShadow(fragPositionLightSpace / fragPositionLightSpace.w);
  outColor = vec4(checkerTexture(uv) * shadow, 1);
}
