#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(push_constant) uniform ShaderTime
{
    float time_seconds;
};

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec2 vertex_uv;
layout(location = 2) in vec4 vertex_color;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main()
{
    gl_Position = vec4(vertex_position, 0.0, 1.0);
    fragUV      = vertex_uv;
    fragColor   = vertex_color;
}
