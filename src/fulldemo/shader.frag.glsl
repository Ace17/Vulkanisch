#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec4 fragPositionLightSpace;

layout(location = 0) out vec4 outColor;

// Scene DescriptorSet (set=0), Shadow Map (binding=1)
layout(set=0, binding=1) uniform sampler2D shadowMapSampler;

// Material DescriptorSet (set=0), MaterialParams (binding=0)
layout(set=1, binding=0, std140) uniform MaterialParamsDescriptorSet
{
  vec4 diffuse;
  vec4 emissive;
} MaterialParams;

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
  vec3 ambient = vec3(1, 1, 1) * 0.04;
  vec3 lightVector = normalize(vec3(1, 1, 1));
  float light = max(dot(inNormal, lightVector), 0) * 0.5;
  float shadow = computeShadow(fragPositionLightSpace / fragPositionLightSpace.w);

  vec3 totalLight = vec3(0, 0, 0);

  totalLight += MaterialParams.diffuse.rgb * ambient.rgb;
  totalLight += MaterialParams.diffuse.rgb * shadow * light;
  totalLight += MaterialParams.emissive.rgb;

  outColor = vec4(totalLight, 1);
}
