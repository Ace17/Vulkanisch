#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D myColorSampler;

void main()
{
    outColor = texture(myColorSampler, uv);
}

// vim: syntax=glsl
