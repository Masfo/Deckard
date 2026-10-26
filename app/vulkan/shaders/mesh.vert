#version 450
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(set = 1, binding = 0, std140) uniform Scene
{
    mat4 view_projection;
    vec4 light_position_intensity;
    vec4 light_color_radius;
} scene;

layout(push_constant) uniform Push
{
    mat4 model;  // world transform, column-major
    mat3 normal; // inverse-transpose of the model matrix
    vec4 color;  // straight RGBA
} pc;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec4 v_color;
layout(location = 3) out vec3 v_world_position;

void main()
{
    vec4 world_position = pc.model * vec4(in_position, 1.0);
    gl_Position         = scene.view_projection * world_position;
    v_world_position    = world_position.xyz;
    v_normal            = pc.normal * in_normal;
    v_uv                = in_uv;
    v_color             = pc.color;
}
