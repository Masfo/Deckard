#version 450

#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

void main()
{
  vec2 seed = gl_FragCoord.xy + fragPos * 1000.0;
  vec3 random_color = vec3(
        hash(seed),
        hash(seed + 17.0),
        hash(seed + 31.0)
    );

    //outColor   = vec4(random_color, 1.0);
	
    outColor = vec4(fragColor, 1.0);
}
