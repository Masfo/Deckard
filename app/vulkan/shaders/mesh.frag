#version 450
layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec4 v_color;
layout(location = 3) in vec3 v_world_position;

layout(set = 0, binding = 0) uniform sampler2D image_texture;
layout(set = 1, binding = 0, std140) uniform Scene
{
    mat4 view_projection;
    vec4 light_position_intensity;
    vec4 light_color_radius;
} scene;

layout(location = 0) out vec4 out_color;

void main()
{
	vec3  to_light = scene.light_position_intensity.xyz - v_world_position;
	float distance = length(to_light);
	float falloff  = max(1.0 - distance / max(scene.light_color_radius.w, 0.001), 0.0);
	float diffuse  = max(dot(normalize(v_normal), to_light / max(distance, 0.0001)), 0.0);

	vec4 texel = texture(image_texture, v_uv);
	vec3 lighting = vec3(0.12) + scene.light_color_radius.rgb * scene.light_position_intensity.w
					 * falloff * falloff * diffuse;
	out_color = vec4(v_color.rgb * (texel.rgb * lighting + v_color.a), texel.a);
}
