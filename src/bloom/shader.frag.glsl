#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 outColor;

layout(set=0, binding=1) uniform sampler2D myColorSampler;

void main()
{
    vec3 lightVector = normalize(vec3(0, 0, 1));

    float amount = max(0, dot(lightVector, normal)) * 2;
    float ambient = 0.05;

    outColor = vec4(1.0) * (amount + ambient);
}

