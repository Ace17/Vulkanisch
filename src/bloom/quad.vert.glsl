#version 450

layout(location = 0) out vec2 outUv;

const vec2 v[6] =
{
  { -1, -1 },
  { +1, +1 },
  { -1, +1 },

  { -1, -1 },
  { +1, -1 },
  { +1, +1 },
};

void main()
{
  vec2 pos = v[gl_VertexIndex];
  outUv = pos * 0.5 + 0.5;
  outUv.y = 1 - outUv.y;
  gl_Position = vec4(pos, 0, 1);
}

