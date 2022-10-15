#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D inputPicture;

void main()
{
  vec3 result = texture(inputPicture, uv).rgb;

  if(length(result) < 0.95)
    result = vec3(0);

  outColor = vec4(result, 1.0);
}

