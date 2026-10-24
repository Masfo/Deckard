#version 450

layout(set = 0, binding = 0) uniform sampler2D imageTexture;
layout(push_constant) uniform ShaderTime
{
    float time_seconds;
};

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = /*fragColor */ texture(imageTexture, fragUV);
}
