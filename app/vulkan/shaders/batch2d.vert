#version 450
layout(location = 0) in vec2 in_pos;     // pixels, origin top-left
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;   // R8G8B8A8_UNORM -> 0..1
layout(push_constant) uniform Push { vec2 scale; } pc;   // 2 / framebuffer size
layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_uv;

void main()
{
    gl_Position = vec4(in_pos * pc.scale - 1.0, 0.0, 1.0);   // Vulkan NDC y is down, so no flip
    v_color     = in_color;
    v_uv        = in_uv;
}
