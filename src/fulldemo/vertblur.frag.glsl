#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D unusedPicture;
layout(set=0, binding=1) uniform sampler2D inputPicture;

void main()
{
  float weight[5];

  weight[0] = 0.227027;
  weight[1] = 0.1945946;
  weight[2] = 0.1216216;
  weight[3] = 0.054054;
  weight[4] = 0.016216;

  float blurScale = 1.5;

  // gets size of single texel
  vec2 tex_offset = blurScale / textureSize(inputPicture, 0);

  vec3 result = vec3(0);

  result += texture(inputPicture, uv).rgb * weight[0];
  for(int i = 1; i < 5; ++i)
  {
    result += weight[i] * texture(inputPicture, uv + vec2(tex_offset.x * i, 0)).rgb;
    result += weight[i] * texture(inputPicture, uv - vec2(tex_offset.x * i, 0)).rgb;
  }

  outColor = vec4(result, 1.0);
}


